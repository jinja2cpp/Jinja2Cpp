#ifndef JINJA2CPP_SRC_RENDER_WORKSPACE_H
#define JINJA2CPP_SRC_RENDER_WORKSPACE_H

#include "internal_value.h"
#include "slot_frame.h"

#include <boost/core/span.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace jinja2
{
struct GlobalsSnapshot;

namespace detail
{
struct LoopFrame;
// A frame is a refcounted ValueObject, as the `loop` value it backs shares it (statements.cpp)
void intrusive_ptr_add_ref(const LoopFrame* frame) noexcept; // NOLINT(readability-identifier-naming): boost::intrusive_ptr's hook
void intrusive_ptr_release(const LoopFrame* frame) noexcept; // NOLINT(readability-identifier-naming): boost::intrusive_ptr's hook
} // namespace detail

// What the renders on one thread reuse from one render to the next (docs/design/
// perf-design-overview.md): the slot frames of unit calls, the frames of finished loops
// (docs/tasks/0133), the environment's converted globals (docs/tasks/0139) and the emptied
// tables of the params and top scope of a render (docs/tasks/0160).
// Frames are taken and given back in stack order, so a render started inside another one
// on the same thread (a user callable rendering a template) stacks its frames on top.
class RenderWorkspace
{
public:
    // The workspace of this thread. Only the library's own code uses it: a program linked to
    // the shared library would see a workspace of its own
    static RenderWorkspace& ForThisThread()
    {
        thread_local RenderWorkspace workspace;
        return workspace;
    }

    RenderWorkspace();
    RenderWorkspace(const RenderWorkspace&) = delete;
    RenderWorkspace(RenderWorkspace&&) = delete;
    RenderWorkspace& operator=(const RenderWorkspace&) = delete;
    RenderWorkspace& operator=(RenderWorkspace&&) = delete;
    ~RenderWorkspace();

    // A frame of `layout.size` unbound slots. Its slots stay where they are until it is given
    // back, whatever is taken after it. Inline while the current chunk has room: a macro
    // call takes and gives back a frame each time (docs/tasks/0147)
    SlotFrame Take(UnitLayout layout)
    {
        if (m_current < m_chunks.size() && m_chunks[m_current].size - m_chunks[m_current].used >= layout.size)
        {
            return TakeFromCurrent(layout);
        }
        return TakeFromNextChunk(layout);
    }
    // Gives back the frame taken last, its slots all unbound
    void Release(const SlotFrame& frame)
    {
        assert(!m_frames.empty() && m_frames.back().frame.handle == frame.handle);
        assert(std::none_of(frame.slots.begin(), frame.slots.end(), [](const Slot& slot) { return slot.IsBound(); }));
        // The statements that bind slots unbind them on every exit; this keeps a release build
        // from handing a value to the next frame if one did not
        for (auto& slot : frame.slots)
        {
            if (slot.IsBound())
            {
                slot.Unbind();
            }
        }
        m_current = m_frames.back().chunk;
        m_chunks[m_current].used -= frame.slots.size();
        m_frames.pop_back();
    }
    // The frame `handle` names; throws when it was given back
    [[nodiscard]] SlotFrame Resolve(FrameHandle handle) const;
    [[nodiscard]] std::size_t FrameCount() const { return m_frames.size(); }

    // The frames of finished loops, newest last (statements.cpp); never more than
    // MaxLoopFrames, and reserved, so that putting one back, done on unwinding too, never
    // allocates
    static constexpr std::size_t MaxLoopFrames = 16;
    std::vector<boost::intrusive_ptr<detail::LoopFrame>>& LoopFrames() { return m_loopFrames; }
    // The globals converted last on this thread (template_impl.h)
    std::shared_ptr<const GlobalsSnapshot>& Globals() { return m_globals; }

    // The tables of the params and the top scope of a render (docs/tasks/0160)
    struct RenderMaps
    {
        InternalValueMap params;
        InternalValueMap topScope;
    };
    // Empty maps for a render, lent until ReturnMaps: with the tables and nodes an earlier
    // render left in them, so that a steady render allocates nothing for them. A render
    // started inside this one is lent the next pair
    RenderMaps& LendMaps()
    {
        if (m_lentMaps < m_maps.size())
        {
            return *m_maps[m_lentMaps++];
        }
        return LendNewMaps();
    }
    // Takes back the maps lent last and empties them
    void ReturnMaps(RenderMaps& maps) noexcept
    {
        assert(m_lentMaps != 0 && m_maps[m_lentMaps - 1].get() == &maps);
        --m_lentMaps;
        // A render that set no name and got no params has nothing to empty
        if ((maps.params.mask() | maps.topScope.mask()) != 0)
        {
            EmptyMaps(maps, m_lentMaps < MaxKeptMaps);
        }
    }
    // The tables kept for the next render have at most 1024 buckets: about 800 names. A
    // bigger one is let go, so that one huge render does not hold memory on the thread
    static constexpr std::size_t MaxMapMask = 1023;
    // The tables of the renders nested deeper than this are let go too, so that a deep
    // recursion through user callables leaves no tables on the thread
    static constexpr std::size_t MaxKeptMaps = 4;

private:
    struct Chunk
    {
        std::unique_ptr<Slot[]> slots;
        std::size_t size = 0;
        std::size_t used = 0;
    };
    struct Record
    {
        SlotFrame frame;
        std::size_t chunk = 0;
    };

    SlotFrame TakeFromCurrent(UnitLayout layout)
    {
        auto& chunk = m_chunks[m_current];
        SlotFrame frame;
        frame.slots = boost::span<Slot>(chunk.slots.get() + chunk.used, layout.size);
        frame.handle = { static_cast<std::uint32_t>(m_frames.size()), ++m_lastGeneration };
        frame.unit = layout.id;
        m_frames.push_back({ frame, m_current });
        chunk.used += layout.size;
        return frame;
    }
    // Moves to the first chunk after the current one with room, allocating one at the end
    // when none has
    SlotFrame TakeFromNextChunk(UnitLayout layout);
    RenderMaps& LendNewMaps();
    static void EmptyMaps(RenderMaps& maps, bool keepTables) noexcept;

    // The smallest chunk: a template with a few nested loops and macros fits in it
    static constexpr std::size_t FirstChunkSize = 256;

    std::vector<Chunk> m_chunks;
    // The chunk frames are taken from
    std::size_t m_current = 0;
    std::vector<Record> m_frames;
    std::uint32_t m_lastGeneration = 0;
    std::vector<boost::intrusive_ptr<detail::LoopFrame>> m_loopFrames;
    std::shared_ptr<const GlobalsSnapshot> m_globals;
    // Where the maps live never changes, so that the maps lent to a render stay in place
    // while a render inside it is lent others
    std::vector<std::unique_ptr<RenderMaps>> m_maps;
    std::size_t m_lentMaps = 0;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDER_WORKSPACE_H

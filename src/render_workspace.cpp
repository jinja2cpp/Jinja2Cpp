#include "render_workspace.h"
#include "internal_value.h"
#include "slot_frame.h"

#include <boost/core/span.hpp>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>

namespace jinja2
{
inline namespace v2
{
RenderWorkspace::RenderWorkspace()
{
    m_loopFrames.reserve(MaxLoopFrames);
}

RenderWorkspace::~RenderWorkspace() = default;

SlotFrame RenderWorkspace::TakeFromNextChunk(UnitLayout layout)
{
    const std::size_t size = layout.size;
    // A frame never spans two chunks, and a chunk never moves, so the frames taken before
    // keep their slots
    while (m_current < m_chunks.size() && m_chunks[m_current].size - m_chunks[m_current].used < size)
    {
        ++m_current;
    }
    if (m_current == m_chunks.size())
    {
        const std::size_t last = m_chunks.empty() ? 0 : m_chunks.back().size;
        Chunk chunk;
        chunk.size = std::max({ FirstChunkSize, last * 2, size });
        chunk.slots = std::make_unique<Slot[]>(chunk.size);
        m_chunks.push_back(std::move(chunk));
    }
    return TakeFromCurrent(layout);
}

SlotFrame RenderWorkspace::Resolve(FrameHandle handle) const
{
    if (handle.depth >= m_frames.size() || m_frames[handle.depth].frame.handle != handle)
    {
        throw std::logic_error("jinja2cpp: a slot frame was used after its call ended");
    }
    return m_frames[handle.depth].frame;
}
RenderWorkspace::RenderMaps& RenderWorkspace::LendNewMaps()
{
    m_maps.push_back(std::make_unique<RenderMaps>());
    return *m_maps[m_lentMaps++];
}

bool RenderWorkspace::ReleaseCaches() noexcept
{
    if (m_lentMaps != 0 || !m_frames.empty())
    {
        return false;
    }
    // Moved out first, so that the workspace is empty before the destructors of what they hold
    // run host code. m_lastGeneration goes on counting: a handle of a freed frame never resolves
    const auto chunks = std::move(m_chunks);
    const auto frames = std::move(m_frames);
    const auto maps = std::move(m_maps);
    const auto globals = std::move(m_globals);
    m_chunks.clear();
    m_frames.clear();
    m_maps.clear();
    m_current = 0;
    // One at a time, each destroyed after it left the vector; the capacity stays reserved (LoopFrames)
    while (!m_loopFrames.empty())
    {
        const auto frame = std::move(m_loopFrames.back());
        m_loopFrames.pop_back();
    }
    return true;
}

bool RenderWorkspace::ReleaseThisThread() noexcept
{
    RenderWorkspace* workspace = nullptr;
    try
    {
        workspace = &ForThisThread();
    }
    catch (...)
    {
        // Out of memory making the workspace of a thread that never rendered: nothing to free
        return true;
    }
    return workspace->ReleaseCaches();
}

void RenderWorkspace::EmptyMaps(RenderMaps& maps, bool keepTables) noexcept
{
    for (auto* map : { &maps.params, &maps.topScope })
    {
        if (!keepTables || map->mask() > MaxMapMask)
        {
            // Moving out takes the table and nodes; assigning an empty map would keep them
            const InternalValueMap dropped(std::move(*map));
        }
        else
        {
            map->clear();
        }
    }
}
} // namespace v2
} // namespace jinja2

#include "render_workspace.h"
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
RenderWorkspace::RenderWorkspace()
{
    m_loopFrames.reserve(MaxLoopFrames);
}

RenderWorkspace::~RenderWorkspace() = default;

SlotFrame RenderWorkspace::Take(UnitLayout layout)
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
    auto& chunk = m_chunks[m_current];
    SlotFrame frame;
    frame.slots = boost::span<Slot>(chunk.slots.get() + chunk.used, size);
    frame.handle = { static_cast<std::uint32_t>(m_frames.size()), ++m_lastGeneration };
    frame.unit = layout.id;
    m_frames.push_back({ frame, m_current });
    chunk.used += size;
    return frame;
}

void RenderWorkspace::Release(const SlotFrame& frame)
{
    assert(!m_frames.empty() && m_frames.back().frame.handle == frame.handle);
    assert(std::none_of(frame.slots.begin(), frame.slots.end(), [](const Slot& slot) { return slot.IsBound(); }));
    const auto& record = m_frames.back();
    m_current = record.chunk;
    m_chunks[m_current].used -= frame.slots.size();
    m_frames.pop_back();
}

SlotFrame RenderWorkspace::Resolve(FrameHandle handle) const
{
    if (handle.depth >= m_frames.size() || m_frames[handle.depth].frame.handle != handle)
    {
        throw std::logic_error("jinja2cpp: a slot frame was used after its call ended");
    }
    return m_frames[handle.depth].frame;
}
} // namespace jinja2

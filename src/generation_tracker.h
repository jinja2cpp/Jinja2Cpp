#ifndef JINJA2CPP_SRC_GENERATION_TRACKER_H
#define JINJA2CPP_SRC_GENERATION_TRACKER_H

#include "internal_value.h"
#include "out_stream.h"
#include "render_context.h"

#include <jinja2cpp/template.h>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// What a render with RenderOptions::generationSpans keeps to place `{% generation %}` blocks (docs/tasks/0377). It lives
// on the render's stack and the render's callback points to it; the callback of a render a host callable starts has no
// tracker, so that render records nothing
struct GenerationTracker
{
    // The stream of the render's output, whose Written() minus `base` is the position of the next character
    const OutStream* root = nullptr;
    // What the target held before this render, which the offsets do not count
    size_t base = 0;
    std::vector<GenerationSpan>* spans = nullptr;
    // Depth of GenerationFreeze, and where the output stood when the first level began
    unsigned frozen = 0;
    size_t frozenAt = 0;

    [[nodiscard]] size_t RootPos() const { return root->Written() - base; }

    // Where a block that starts now begins: transformers measures what the top-level generator has yielded, and a
    // buffered part (a macro's body, a call block) has not been yielded yet
    [[nodiscard]] size_t StartPos() const { return frozen ? frozenAt : RootPos(); }

    void Push(size_t start, size_t end) const
    {
        CheckSequenceSize(static_cast<uint64_t>(spans->size()) + 1, "the generation spans");
        spans->push_back({ start, end });
    }
};

// Marks a region that Jinja2 renders into a buffer (a macro's result, a call block's caller, a recursive loop) but
// Jinja2C++ streams: the blocks inside begin where the region began. A no-op without a tracker
class GenerationFreeze
{
public:
    explicit GenerationFreeze(RenderContext& context)
        : m_tracker(context.GetRendererCallback()->GetGenerationTracker())
    {
        if (m_tracker)
        {
            m_spans = m_tracker->spans->size();
            if (m_tracker->frozen++ == 0)
            {
                m_tracker->frozenAt = m_tracker->RootPos();
            }
        }
    }
    GenerationFreeze(const GenerationFreeze&) = delete;
    GenerationFreeze& operator=(const GenerationFreeze&) = delete;
    GenerationFreeze(GenerationFreeze&&) = delete;
    GenerationFreeze& operator=(GenerationFreeze&&) = delete;
    ~GenerationFreeze()
    {
        if (m_tracker)
        {
            --m_tracker->frozen;
        }
    }

    // Drops the spans recorded since the freeze began: the call it covers stopped (docs/tasks/0479), and Jinja2
    // drops what the call buffered
    void Discard() const
    {
        if (m_tracker)
        {
            m_tracker->spans->resize(m_spans);
        }
    }

private:
    GenerationTracker* m_tracker;
    size_t m_spans = 0;
};
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_GENERATION_TRACKER_H

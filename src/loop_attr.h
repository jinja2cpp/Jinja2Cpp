#ifndef JINJA2CPP_SRC_LOOP_ATTR_H
#define JINJA2CPP_SRC_LOOP_ATTR_H

#include <cstdint>
#include <string_view>

namespace jinja2
{
// An attribute of a for loop's `loop` object. `loop.<name>` in the template is resolved to
// one of these at Load, so a render reads the attribute without comparing strings
// (docs/design/perf-design-overview.md row 17; 0117 P1)
enum class LoopAttr : std::uint8_t
{
    None,
    Index,
    Index0,
    RevIndex,
    RevIndex0,
    First,
    Last,
    Length,
    Depth,
    Depth0,
    PrevItem,
    NextItem,
    Cycle,
    Changed,
    // `loop(...)` of a recursive loop: not an attribute name
    Call
};

// The attribute `name` of `loop`, None when it is not one
inline LoopAttr FindLoopAttr(std::string_view name)
{
    struct Entry
    {
        std::string_view name;
        LoopAttr attr;
    };
    static constexpr Entry entries[] = {
        { "index", LoopAttr::Index },
        { "index0", LoopAttr::Index0 },
        { "revindex", LoopAttr::RevIndex },
        { "revindex0", LoopAttr::RevIndex0 },
        { "first", LoopAttr::First },
        { "last", LoopAttr::Last },
        { "length", LoopAttr::Length },
        { "depth", LoopAttr::Depth },
        { "depth0", LoopAttr::Depth0 },
        { "previtem", LoopAttr::PrevItem },
        { "nextitem", LoopAttr::NextItem },
        { "cycle", LoopAttr::Cycle },
        { "changed", LoopAttr::Changed },
    };
    for (const auto& entry : entries)
    {
        if (entry.name == name)
        {
            return entry.attr;
        }
    }
    return LoopAttr::None;
}
} // namespace jinja2

#endif // JINJA2CPP_SRC_LOOP_ATTR_H

#ifndef JINJA2CPP_SRC_STABLE_SORT_VALUES_H
#define JINJA2CPP_SRC_STABLE_SORT_VALUES_H

#include <algorithm>
#include <iterator>

namespace jinja2
{
inline namespace v2
{
// std::stable_sort for comparators that can throw on template values (None, mixed types).
// Python's sorted() never compares fewer than two items, so a lone item with an unorderable
// key sorts; the checking standard library (JINJA2CPP_DEBUG_CONTAINERS, docs/tasks/0344)
// compares an element with itself in a one-element range and would report the throw.
template<typename Iterator, typename Compare>
void StableSortValues(Iterator first, Iterator last, Compare comp)
{
    if (std::distance(first, last) > 1)
    {
        std::stable_sort(first, last, comp);
    }
}
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_STABLE_SORT_VALUES_H

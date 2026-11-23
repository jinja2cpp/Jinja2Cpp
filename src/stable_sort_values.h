#ifndef JINJA2CPP_SRC_STABLE_SORT_VALUES_H
#define JINJA2CPP_SRC_STABLE_SORT_VALUES_H

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// Sorted runs start this long; insertion sort is cheaper than merging below it.
constexpr std::ptrdiff_t StableSortRunLength = 16;

// Guarded insertion sort of [first + begin, first + end): the scan stops at begin whatever
// comp answers, unlike libstdc++'s unguarded insertion, which relies on comp(x, *first) being
// consistent with an earlier answer and walks below first when it is not.
template<typename Iterator, typename Compare>
void StableSortInsertRun(Iterator first, std::ptrdiff_t begin, std::ptrdiff_t end, Compare& comp)
{
    for (std::ptrdiff_t idx = begin + 1; idx < end; ++idx)
    {
        if (!comp(first[idx], first[idx - 1]))
        {
            continue;
        }
        auto moving = std::move(first[idx]);
        std::ptrdiff_t hole = idx;
        try
        {
            do
            {
                first[hole] = std::move(first[hole - 1]);
                --hole;
            } while (hole > begin && comp(moving, first[hole - 1]));
        }
        catch (...)
        {
            first[hole] = std::move(moving);
            throw;
        }
        first[hole] = std::move(moving);
    }
}

// Merges the sorted runs [lo, mid) and [mid, hi). The left run goes to buffer; the write
// position never passes the next unread right element (out = lo + taken left + taken right
// <= right), so no comparator answer can make it write outside [lo, hi).
template<typename Iterator, typename Buffer, typename Compare>
void StableSortMergeRuns(Iterator first, std::ptrdiff_t lo, std::ptrdiff_t mid, std::ptrdiff_t hi, Buffer& buffer, Compare& comp)
{
    if (!comp(first[mid], first[mid - 1]))
    {
        return;
    }
    buffer.clear();
    buffer.insert(buffer.end(), std::make_move_iterator(first + lo), std::make_move_iterator(first + mid));
    const auto leftSize = static_cast<std::ptrdiff_t>(buffer.size());
    std::ptrdiff_t left = 0;
    std::ptrdiff_t right = mid;
    std::ptrdiff_t out = lo;
    try
    {
        while (left < leftSize && right < hi)
        {
            if (comp(first[right], buffer.begin()[left]))
            {
                first[out++] = std::move(first[right++]);
            }
            else
            {
                first[out++] = std::move(buffer.begin()[left++]);
            }
        }
    }
    catch (...)
    {
        // The free slots [out, right) are exactly as many as the unwritten left items.
        std::move(buffer.begin() + left, buffer.end(), first + out);
        throw;
    }
    std::move(buffer.begin() + left, buffer.end(), first + out);
}
} // namespace detail

// Stable sort for comparators that can throw on template values (None, mixed types) or answer
// inconsistently (host objects' own ordering, task 0381). The result is sorted when comp is a
// strict weak order; for any other comp every access stays inside [first, last), and when comp
// throws the range still holds a permutation of its items. O(n log n) comparisons, a buffer of
// fewer than n items, none for ranges up to StableSortRunLength.
// Python's sorted() never compares fewer than two items, so a lone item with an unorderable
// key sorts; the checking standard library (JINJA2CPP_DEBUG_CONTAINERS, docs/tasks/0344)
// would also compare an element with itself in a one-element range.
template<typename Iterator, typename Compare>
void StableSortValues(Iterator first, Iterator last, Compare comp)
{
    const std::ptrdiff_t size = std::distance(first, last);
    if (size < 2)
    {
        return;
    }
    for (std::ptrdiff_t begin = 0; begin < size; begin += detail::StableSortRunLength)
    {
        detail::StableSortInsertRun(first, begin, std::min(begin + detail::StableSortRunLength, size), comp);
    }
    if (size <= detail::StableSortRunLength)
    {
        return;
    }
    std::ptrdiff_t widest = detail::StableSortRunLength;
    while (widest * 2 < size)
    {
        widest *= 2;
    }
    std::vector<typename std::iterator_traits<Iterator>::value_type> buffer;
    buffer.reserve(static_cast<std::size_t>(widest));
    for (std::ptrdiff_t width = detail::StableSortRunLength; width < size; width *= 2)
    {
        for (std::ptrdiff_t lo = 0; lo + width < size; lo += 2 * width)
        {
            detail::StableSortMergeRuns(first, lo, lo + width, std::min(lo + 2 * width, size), buffer, comp);
        }
    }
}
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_STABLE_SORT_VALUES_H

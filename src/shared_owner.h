#ifndef JINJA2CPP_SRC_SHARED_OWNER_H
#define JINJA2CPP_SRC_SHARED_OWNER_H

#include <memory>

namespace jinja2
{
inline namespace v2
{

// Whether `owner` is the only owner of its object, with what the other owners did to the object
// ordered before what the caller does next (docs/tasks/0339). shared_ptr::use_count() is a relaxed
// load, which does not synchronize with the release of the last other owner: an in-place write
// after it alone races with that owner's last read. A copy dropped again is an acq_rel decrement
// of the same count, which reads the last value in its modification order and so synchronizes
// with every earlier release. An acquire fence would do as well, but TSan does not model fences.
// Two atomic operations more than use_count(): for paths off the per-node render path. Storage
// the library makes itself takes a ValueObject's count instead, which has an acquire load
template<typename T>
bool IsSoleOwner(const std::shared_ptr<T>& owner) noexcept
{
    if (owner.use_count() != 1) // use_count: a hint only, the decrement below orders the drop
    {
        return false;
    }
    std::shared_ptr<T>(owner).reset();
    return true;
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SHARED_OWNER_H

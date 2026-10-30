#ifndef JINJA2CPP_SRC_SHARED_OWNER_H
#define JINJA2CPP_SRC_SHARED_OWNER_H

#include <atomic>
#include <memory>

namespace jinja2
{
inline namespace v2
{

// GCC and Clang each say it their own way
#ifdef __SANITIZE_THREAD__
#define JINJA2CPP_SHARED_OWNER_TSAN 1
#endif
#ifdef __has_feature
#if __has_feature(thread_sanitizer)
#define JINJA2CPP_SHARED_OWNER_TSAN 1
#endif
#endif

// Whether `owner` is the only owner of its object, with what the other owners did to the object
// ordered before what the caller does next (docs/tasks/0339). shared_ptr::use_count() is a relaxed
// load, which does not synchronize with the release of the last other owner: an in-place write
// after it alone races with that owner's last read. The last other owner's drop is a release
// decrement, so an acquire fence after a load that read its value synchronizes with it
// ([atomics.fences]/2), at the cost of nothing on x86. TSan does not model fences, so a TSan
// build drops a copy instead: an acq_rel decrement of the same count, which reads the last value
// in its modification order and so synchronizes with every earlier release. Storage the library
// makes itself takes a ValueObject's count instead, which has an acquire load
template<typename T>
bool IsSoleOwner(const std::shared_ptr<T>& owner) noexcept
{
    if (owner.use_count() != 1) // use_count: a relaxed load, ordered by what follows
    {
        return false;
    }
#ifdef JINJA2CPP_SHARED_OWNER_TSAN
    std::shared_ptr<T>(owner).reset();
#else
    std::atomic_thread_fence(std::memory_order_acquire);
#endif
    return true;
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SHARED_OWNER_H

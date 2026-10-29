// Self-check of the checking standard library build (JINJA2CPP_DEBUG_CONTAINERS, docs/tasks/0344):
// a misuse that plain ASan cannot see must abort. If these tests pass in a plain build they are
// skipped, so the job cannot go green with the checks silently off.
#include "gtest/gtest.h"

#include <cstddef> // IWYU pragma: keep (used in the libc++ branch)
#include <vector>  // IWYU pragma: keep (used in the checking-library branches)

namespace
{

#ifdef _GLIBCXX_DEBUG
// The vector keeps its buffer, so the stale iterator points into live memory: only the safe
// iterator knows that the erase invalidated it.
void UseInvalidatedIterator()
{
    std::vector<int> v;
    v.reserve(8);
    v.push_back(1);
    v.push_back(2);
    auto it = v.begin();
    v.erase(v.begin());
    volatile int sink = *it;
    (void)sink;
}
#define JINJA2CPP_HAS_CONTAINER_CHECKS 1
#elif defined(_LIBCPP_HARDENING_MODE) && _LIBCPP_HARDENING_MODE == _LIBCPP_HARDENING_MODE_DEBUG
// An index one past the end, inside the capacity: hardened operator[] asserts
void UseInvalidatedIterator()
{
    std::vector<int> v;
    v.reserve(8);
    v.push_back(1);
    volatile std::size_t index = 1;
    volatile int sink = v[index];
    (void)sink;
}
#define JINJA2CPP_HAS_CONTAINER_CHECKS 1
#endif

} // namespace

TEST(DebugContainersTest, MisuseAborts)
{
#ifdef JINJA2CPP_HAS_CONTAINER_CHECKS
    EXPECT_DEATH(UseInvalidatedIterator(), "");
#else
    GTEST_SKIP() << "not a JINJA2CPP_DEBUG_CONTAINERS build";
#endif
}

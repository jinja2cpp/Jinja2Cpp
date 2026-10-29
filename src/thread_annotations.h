#ifndef JINJA2CPP_SRC_THREAD_ANNOTATIONS_H
#define JINJA2CPP_SRC_THREAD_ANNOTATIONS_H

// Lock discipline that Clang checks at compile time (-Wthread-safety, docs/tasks/0343): a member marked
// JINJA2CPP_GUARDED_BY(m) is read only under a shared or exclusive lock of m and written only under an
// exclusive one, and a function marked JINJA2CPP_REQUIRES(m) is called only with m held. The standard
// library does not annotate its mutexes, so the library locks through the wrappers below. Other compilers
// see nothing. The analysis does not check constructors and destructors, and it does not see thread
// confinement or atomics: the borrowed-cell protocol is checked by the TLA+ model and TSan instead
// (docs/design/clang-static-analysis-evaluation.md).

#include <functional>
#include <mutex>
#include <shared_mutex>

#if defined(__clang__)
#define JINJA2CPP_THREAD_ANNOTATION(x) __attribute__((x))
#else
#define JINJA2CPP_THREAD_ANNOTATION(x)
#endif

#define JINJA2CPP_CAPABILITY(x) JINJA2CPP_THREAD_ANNOTATION(capability(x))
#define JINJA2CPP_SCOPED_CAPABILITY JINJA2CPP_THREAD_ANNOTATION(scoped_lockable)
#define JINJA2CPP_GUARDED_BY(x) JINJA2CPP_THREAD_ANNOTATION(guarded_by(x))
#define JINJA2CPP_REQUIRES(...) JINJA2CPP_THREAD_ANNOTATION(requires_capability(__VA_ARGS__))
#define JINJA2CPP_ACQUIRE(...) JINJA2CPP_THREAD_ANNOTATION(acquire_capability(__VA_ARGS__))
#define JINJA2CPP_ACQUIRE_SHARED(...) JINJA2CPP_THREAD_ANNOTATION(acquire_shared_capability(__VA_ARGS__))
#define JINJA2CPP_RELEASE(...) JINJA2CPP_THREAD_ANNOTATION(release_capability(__VA_ARGS__))
#define JINJA2CPP_RELEASE_SHARED(...) JINJA2CPP_THREAD_ANNOTATION(release_shared_capability(__VA_ARGS__))
#define JINJA2CPP_RELEASE_GENERIC(...) JINJA2CPP_THREAD_ANNOTATION(release_generic_capability(__VA_ARGS__))
#define JINJA2CPP_TRY_ACQUIRE(...) JINJA2CPP_THREAD_ANNOTATION(try_acquire_capability(__VA_ARGS__))
#define JINJA2CPP_TRY_ACQUIRE_SHARED(...) JINJA2CPP_THREAD_ANNOTATION(try_acquire_shared_capability(__VA_ARGS__))
// For a function the analysis cannot follow; say why next to it
#define JINJA2CPP_NO_THREAD_SAFETY_ANALYSIS JINJA2CPP_THREAD_ANNOTATION(no_thread_safety_analysis)

namespace jinja2
{
inline namespace v2
{
namespace detail
{

class JINJA2CPP_CAPABILITY("mutex") Mutex
{
public:
    void lock() JINJA2CPP_ACQUIRE() { m_mutex.lock(); }
    void unlock() JINJA2CPP_RELEASE() { m_mutex.unlock(); }
    bool try_lock() JINJA2CPP_TRY_ACQUIRE(true) { return m_mutex.try_lock(); }

private:
    std::mutex m_mutex;
};

class JINJA2CPP_CAPABILITY("mutex") SharedMutex
{
public:
    void lock() JINJA2CPP_ACQUIRE() { m_mutex.lock(); }
    void unlock() JINJA2CPP_RELEASE() { m_mutex.unlock(); }
    bool try_lock() JINJA2CPP_TRY_ACQUIRE(true) { return m_mutex.try_lock(); }
    void lock_shared() JINJA2CPP_ACQUIRE_SHARED() { m_mutex.lock_shared(); }
    void unlock_shared() JINJA2CPP_RELEASE_SHARED() { m_mutex.unlock_shared(); }
    bool try_lock_shared() JINJA2CPP_TRY_ACQUIRE_SHARED(true) { return m_mutex.try_lock_shared(); }

private:
    std::shared_timed_mutex m_mutex;
};

// An exclusive lock of a Mutex or a SharedMutex for the scope
template<typename M>
class JINJA2CPP_SCOPED_CAPABILITY WriterLock
{
public:
    explicit WriterLock(M& mutex) JINJA2CPP_ACQUIRE(mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock();
    }
    WriterLock(const WriterLock&) = delete;
    WriterLock(WriterLock&&) = delete;
    WriterLock& operator=(const WriterLock&) = delete;
    WriterLock& operator=(WriterLock&&) = delete;
    ~WriterLock() JINJA2CPP_RELEASE() { m_mutex.unlock(); }

private:
    M& m_mutex;
};

// A shared lock of a SharedMutex for the scope
class JINJA2CPP_SCOPED_CAPABILITY ReaderLock
{
public:
    explicit ReaderLock(SharedMutex& mutex) JINJA2CPP_ACQUIRE_SHARED(mutex)
        : m_mutex(mutex)
    {
        m_mutex.lock_shared();
    }
    ReaderLock(const ReaderLock&) = delete;
    ReaderLock(ReaderLock&&) = delete;
    ReaderLock& operator=(const ReaderLock&) = delete;
    ReaderLock& operator=(ReaderLock&&) = delete;
    ~ReaderLock() JINJA2CPP_RELEASE_GENERIC() { m_mutex.unlock_shared(); }

private:
    SharedMutex& m_mutex;
};

// Shared locks of two distinct SharedMutexes for the scope, taken in address order, so that two threads
// locking the same pair never wait on each other behind a writer
class JINJA2CPP_SCOPED_CAPABILITY ReaderLockPair
{
public:
    ReaderLockPair(SharedMutex& first, SharedMutex& second) JINJA2CPP_ACQUIRE_SHARED(first, second)
        : m_low(std::less<SharedMutex*>()(&first, &second) ? first : second)
        , m_high(std::less<SharedMutex*>()(&first, &second) ? second : first)
    {
        m_low.lock_shared();
        try
        {
            m_high.lock_shared();
        }
        catch (...)
        {
            m_low.unlock_shared();
            throw;
        }
    }
    ReaderLockPair(const ReaderLockPair&) = delete;
    ReaderLockPair(ReaderLockPair&&) = delete;
    ReaderLockPair& operator=(const ReaderLockPair&) = delete;
    ReaderLockPair& operator=(ReaderLockPair&&) = delete;
    ~ReaderLockPair() JINJA2CPP_RELEASE_GENERIC()
    {
        m_high.unlock_shared();
        m_low.unlock_shared();
    }

private:
    SharedMutex& m_low;
    SharedMutex& m_high;
};

} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_THREAD_ANNOTATIONS_H

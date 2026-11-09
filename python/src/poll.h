#ifndef JINJA2CPP_PYTHON_POLL_H
#define JINJA2CPP_PYTHON_POLL_H

#include "common.h"

#include "recursion_guard.h"

#include <functional>

namespace jinja2cpp_py
{
// Ctrl-C during a render or a call of a module's macro: the engine calls this every few steps
// (detail::t_hostPoll), and KeyboardInterrupt stops the run as it stops Python Jinja2's
inline void PollSignals()
{
    if (PyErr_CheckSignals() != 0)
    {
        throw nb::python_error();
    }
}

// Installs PollSignals as the thread's poll for the scope
class HostPollScope
{
public:
    HostPollScope() noexcept
        : m_scope(&Poll())
    {
    }

private:
    static const std::function<void()>& Poll()
    {
        static const std::function<void()> poll(&PollSignals);
        return poll;
    }

    jinja2::HostPollScope m_scope;
};
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_POLL_H

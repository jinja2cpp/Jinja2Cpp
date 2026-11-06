#ifndef JINJA2CPP_PYTHON_POLL_H
#define JINJA2CPP_PYTHON_POLL_H

#include "common.h"

#include "recursion_guard.h"

#include <utility>

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

class HostPollScope
{
public:
    HostPollScope() noexcept
        : m_saved(std::exchange(jinja2::detail::t_hostPoll, &PollSignals))
    {
    }
    ~HostPollScope() { jinja2::detail::t_hostPoll = m_saved; }
    HostPollScope(const HostPollScope&) = delete;
    HostPollScope& operator=(const HostPollScope&) = delete;
    HostPollScope(HostPollScope&&) = delete;
    HostPollScope& operator=(HostPollScope&&) = delete;

private:
    jinja2::detail::HostPoll m_saved;
};
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_POLL_H

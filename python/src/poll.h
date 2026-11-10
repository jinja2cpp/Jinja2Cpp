#ifndef JINJA2CPP_PYTHON_POLL_H
#define JINJA2CPP_PYTHON_POLL_H

#include "common.h"
#include "convert.h"

#include <exception>
#include <functional>

namespace jinja2cpp_py
{
// Ctrl-C during a render or a call of a module's macro: the engine calls this every few steps
// (RenderOptions::poll), and KeyboardInterrupt stops the run as it stops Python Jinja2's. A
// BaseException a callable raised that the engine absorbed (RenderState::abort) stops it too
inline void PollSignals()
{
    if (const auto* render = CurrentRender(); render && render->abort)
    {
        std::rethrow_exception(render->abort);
    }
    if (PyErr_CheckSignals() != 0)
    {
        throw nb::python_error();
    }
}

// The poll a render of the module passes (RenderOptions::poll)
inline const std::function<void()>& RenderPoll()
{
    static const std::function<void()> poll(&PollSignals);
    return poll;
}

// After a run: the BaseException a callable raised, which the engine may have absorbed
inline void RaiseAbort(const RenderState& state)
{
    if (state.abort)
    {
        std::rethrow_exception(state.abort);
    }
}
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_POLL_H

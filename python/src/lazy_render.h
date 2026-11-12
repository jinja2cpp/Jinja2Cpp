#ifndef JINJA2CPP_PYTHON_LAZY_RENDER_H
#define JINJA2CPP_PYTHON_LAZY_RENDER_H

#include "common.h"
#include "convert.h"

#include <fiber.h>

#include <deque>
#include <exception>
#include <memory>
#include <string>
#include <thread>

namespace jinja2cpp_py
{
// Template.generate() rendering on demand (docs/tasks/0457): the engine's render runs on a fiber on
// the caller's thread and switches back at each item its sink gets (RenderOptions::suspension), so
// the template runs as far as the consumer asks, and its callables run on the consumer's thread.
//
// - next() on another thread than the first one raises RuntimeError (the plan's D2).
// - close() ends the render where it is suspended: its sink raises a private exception there, and the
//   engine unwinds the render on its fiber. On another thread, or once the interpreter exits, the fiber
//   is freed without running anything more, and what its frames hold is leaked.
// - A reference cycle through a suspended render is not collected until it is closed (D4, task 0462):
//   the object does not take part in the garbage collector.
class PyLazyRender
{
public:
    PyLazyRender(nb::object tpl, nb::args args, nb::kwargs kwargs);
    ~PyLazyRender();
    PyLazyRender(const PyLazyRender&) = delete;
    PyLazyRender(PyLazyRender&&) = delete;
    PyLazyRender& operator=(const PyLazyRender&) = delete;
    PyLazyRender& operator=(PyLazyRender&&) = delete;

    // The next item, or None once the render ended; raises what the render raised after the items before it
    nb::object Next();
    void Close();

private:
    class Sink;
    friend class Sink;

    void Run();
    void Resume();
    void Abandon() noexcept;
    // Whether the sink may switch now: no Python frame, Python error or C++ exception of the render is live
    [[nodiscard]] bool CanSwitch() const noexcept;
    nb::object Pop();

    nb::object m_template;
    nb::args m_args;
    nb::kwargs m_kwargs;
    std::unique_ptr<jinja2cpp_fiber::Fiber> m_fiber;
    std::deque<std::string> m_items;
    std::exception_ptr m_error;
    ModuleThreadState m_threadState;
    std::thread::id m_owner;
    const void* m_resumeFrame = nullptr;
    bool m_cancelled = false;
    bool m_done = false;
};

// Whether generate() may render on demand now: fibers work here, the interpreter is one whose C stack
// checks are known to work on a fiber (3.12, 3.13, 3.14.1 and later 3.14, 3.15), greenlet is not
// imported, and fewer than 8 lazy renders run nested on this thread
bool LazyGenerateAvailable();

// Template._lazy(*args, **kwargs): a lazy render of `tpl` with dict(*args, **kwargs), or None where
// generate() renders eagerly (LazyGenerateAvailable, or no memory for a fiber's stack)
nb::object MakeLazyRender(nb::object tpl, nb::args args, nb::kwargs kwargs);

// Registers _LazyRender and the module's switches
void BindLazyRender(nb::module_& module);
} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_LAZY_RENDER_H

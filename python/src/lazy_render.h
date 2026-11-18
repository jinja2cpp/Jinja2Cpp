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
#include <utility>
#include <vector>

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
// - It takes part in the garbage collector (docs/tasks/0462): its traverse reports the template and the
//   arguments, and while the render is suspended, the references the render's params and globals hold on the
//   fiber's stack, so a cycle through a suspended render (a generator stored in a value the template holds)
//   is collected. Its clear closes the render, which unwinds on the fiber and lets go of them; on another
//   thread the fiber is freed as close() frees it there, and what its frames hold stays.
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

    int Traverse(visitproc visit, void* arg) const;
    void Clear() noexcept;

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
    // The items the consumer has not taken yet, and whether each is Markup (docs/tasks/0493)
    std::deque<std::pair<std::string, bool>> m_items;
    // Set by the render before each item (RenderOptions::pieceIsMarkup)
    bool m_markup = false;
    // The objects the values of the render's params and globals hold a reference to, while it runs
    // (PieceOptions::held)
    std::vector<PyObject*> m_held;
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

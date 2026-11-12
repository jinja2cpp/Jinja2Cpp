#include "lazy_render.h"

#include "common.h"
#include "convert.h"
#include "environment.h"

#include <fiber.h>
#include <jinja2cpp/template.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace jinja2cpp_py
{
namespace
{
using jinja2cpp_fiber::Fiber;

// Nested lazy renders a thread runs at most: the next one renders eagerly (each holds a fiber's stack)
constexpr std::size_t MaxNestedLazyRenders = 8;

// The render's sink raises it where the render is suspended to end it there (PyLazyRender::Close)
struct RenderCancelled
{
};

// The lazy renders running on this thread, innermost last: a suspension gives CPython the stack of the
// one it returns to
thread_local std::vector<const Fiber*> t_running;

// generate() renders eagerly once the interpreter exits (atexit) or when a test asks (_set_lazy_generate)
bool g_lazyEnabled = true;
bool g_exiting = false;

// CPython 3.14.1 and later check the C stack against the thread's bounds, and do nothing on another stack; these
// set the bounds of a fiber's (not in the limited API: looked up at run time, as an abi3 module must)
using SetStackProtectionFn = int (*)(PyThreadState*, void*, std::size_t);
using ResetStackProtectionFn = void (*)(PyThreadState*);

struct StackProtection
{
    SetStackProtectionFn set = nullptr;
    ResetStackProtectionFn reset = nullptr;
    // Whether the interpreter is one lazy renders are known to work with
    bool allowed = false;
};

void* FindSymbol(const char* name)
{
#if defined(_WIN32)
    HMODULE python = nullptr;
    // The DLL that holds the interpreter, not python3.dll, which only forwards to it
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(reinterpret_cast<void*>(&PyThreadState_Get)),
                            &python))
    {
        return nullptr;
    }
    return reinterpret_cast<void*>(GetProcAddress(python, name));
#else
    return dlsym(RTLD_DEFAULT, name);
#endif
}

const StackProtection& Protection()
{
    static const StackProtection protection = [] {
        StackProtection result;
        const unsigned long version = Py_Version;
        const unsigned long major = version >> 24U;
        const unsigned long minor = (version >> 16U) & 0xFFU;
        const unsigned long micro = (version >> 8U) & 0xFFU;
        if (major != 3)
        {
            return result;
        }
        if (minor == 12 || minor == 13)
        {
            // Counters sized for a thread's stack, which the fiber's matches (Fiber::DefaultStackSize)
            result.allowed = true;
            return result;
        }
        // 3.14.0 aborts on any call from a foreign stack; later versions need the bounds set. Versions after 3.15
        // are checked before they are admitted
        if ((minor == 14 && micro >= 1) || minor == 15)
        {
            result.set = reinterpret_cast<SetStackProtectionFn>(FindSymbol("PyUnstable_ThreadState_SetStackProtection"));
            result.reset = reinterpret_cast<ResetStackProtectionFn>(FindSymbol("PyUnstable_ThreadState_ResetStackProtection"));
            result.allowed = result.set && result.reset;
        }
        return result;
    }();
    return protection;
}

// CPython checks the stack of the innermost running lazy render, or the thread's
void ProtectStack()
{
    const auto& protection = Protection();
    if (!protection.set)
    {
        return;
    }
    PyThreadState* state = PyThreadState_Get();
    if (t_running.empty())
    {
        protection.reset(state);
        return;
    }
    const Fiber& fiber = *t_running.back();
    if (fiber.StackSize() == 0)
    {
        // A Windows fiber knows its bounds only once it runs: Run() sets them first thing
        return;
    }
    if (protection.set(state, const_cast<void*>(fiber.StackLow()), fiber.StackSize()) != 0)
    {
        // Out of memory in CPython: it keeps checking against the bounds it had
        PyErr_Clear();
    }
}

bool GreenletImported()
{
    PyObject* modules = PyImport_GetModuleDict();
    const int found = PyMapping_HasKeyString(modules, "greenlet");
    return found == 1;
}

const void* CurrentPythonFrame() noexcept
{
    return PyEval_GetFrame();
}

[[noreturn]] void RaiseRuntimeError(const char* message)
{
    PyErr_SetString(PyExc_RuntimeError, message);
    throw nb::python_error();
}

[[noreturn]] void RaiseValueError(const char* message)
{
    PyErr_SetString(PyExc_ValueError, message);
    throw nb::python_error();
}
} // namespace

bool LazyGenerateAvailable()
{
    return g_lazyEnabled && !g_exiting && Fiber::Available() && Protection().allowed && t_running.size() < MaxNestedLazyRenders && !GreenletImported();
}

// The engine's sink of a lazy render: keeps each item and switches back to the consumer
class PyLazyRender::Sink final : public jinja2::OutputSink
{
public:
    explicit Sink(PyLazyRender& owner)
        : m_owner(owner)
    {
    }
    Sink(const Sink&) = delete;
    Sink(Sink&&) = delete;
    Sink& operator=(const Sink&) = delete;
    Sink& operator=(Sink&&) = delete;
    ~Sink() = default;

    void Write(std::string_view text) override
    {
        if (m_owner.m_cancelled)
        {
            throw RenderCancelled();
        }
        m_owner.m_items.emplace_back(text);
        // Where a switch would hand live state to the consumer, the item waits, and the render goes on: correct,
        // not lazy (the engine writes only after a statement, so this does not happen today)
        if (!m_owner.CanSwitch())
        {
            return;
        }
        m_owner.m_fiber->Suspend();
        if (m_owner.m_cancelled)
        {
            throw RenderCancelled();
        }
    }

private:
    PyLazyRender& m_owner;
};

PyLazyRender::PyLazyRender(nb::object tpl, nb::args args, nb::kwargs kwargs)
    : m_template(std::move(tpl))
    , m_args(std::move(args))
    , m_kwargs(std::move(kwargs))
    , m_owner(std::this_thread::get_id())
{
    static_cast<void>(nb::cast<const PyTemplate&>(m_template));
    m_fiber = std::make_unique<Fiber>([this] { Run(); });
}

PyLazyRender::~PyLazyRender()
{
    try
    {
        Close();
    }
    catch (nb::python_error& error)
    {
        error.discard_as_unraisable("closing a jinja2cpp generator");
    }
    catch (...)
    {
        Abandon();
    }
}

void PyLazyRender::Run()
{
    // On the fiber, before any Python runs: its bounds are known here on every platform
    ProtectStack();
    const jinja2::RenderSuspension suspension{ m_fiber->StackLow(), m_fiber->StackSize() };
    Sink sink(*this);
    try
    {
        nb::cast<const PyTemplate&>(m_template).RenderSuspended(sink, m_args, m_kwargs, suspension);
    }
    catch (...)
    {
        // Ended by Close: what the unwinding raised is of no use to anyone
        if (!m_cancelled)
        {
            throw;
        }
    }
}

bool PyLazyRender::CanSwitch() const noexcept
{
    return CurrentPythonFrame() == m_resumeFrame && !PyErr_Occurred() && !std::current_exception() && std::uncaught_exceptions() == 0;
}

void PyLazyRender::Resume()
{
    // The module's per-render thread-locals, the engine's (swapped by the engine around Write) and CPython's
    // stack bounds are the render's while it runs, and the consumer's again when it suspends
    // The push may throw: before the exchange, so a failure leaves the caller's state in place
    t_running.push_back(m_fiber.get());
    ExchangeModuleThreadState(m_threadState);
    ProtectStack();
    m_resumeFrame = CurrentPythonFrame();
    try
    {
        m_fiber->Resume();
    }
    catch (...)
    {
        m_error = std::current_exception();
    }
    t_running.pop_back();
    ProtectStack();
    ExchangeModuleThreadState(m_threadState);
    if (m_fiber->Finished())
    {
        m_done = true;
        m_fiber.reset();
    }
}

nb::object PyLazyRender::Pop()
{
    auto item = MakeStr(m_items.front());
    m_items.pop_front();
    return item;
}

nb::object PyLazyRender::Next()
{
    if (!m_items.empty())
    {
        return Pop();
    }
    if (!m_done)
    {
        if (std::this_thread::get_id() != m_owner)
        {
            RaiseRuntimeError("a jinja2cpp generator renders on the thread that made it: render() the template, or wrap "
                              "list(generate()), to hand the result to another thread (docs/python.md)");
        }
        if (m_fiber->Running())
        {
            RaiseValueError("generator already executing");
        }
        Resume();
        if (!m_items.empty())
        {
            return Pop();
        }
    }
    if (m_error)
    {
        std::rethrow_exception(std::exchange(m_error, nullptr));
    }
    return nb::none();
}

void PyLazyRender::Close()
{
    m_items.clear();
    m_error = nullptr;
    if (m_done)
    {
        return;
    }
    if (!m_fiber->Started())
    {
        m_done = true;
        m_fiber.reset();
        return;
    }
    if (m_fiber->Running())
    {
        RaiseValueError("generator already executing");
    }
    if (std::this_thread::get_id() != m_owner || g_exiting)
    {
        Abandon();
        return;
    }
    m_cancelled = true;
    Resume();
    m_items.clear();
    m_error = nullptr;
    Abandon();
}

void PyLazyRender::Abandon() noexcept
{
    // A suspended fiber runs nothing: its stack goes, and what its frames own stays
    m_done = true;
    m_items.clear();
    m_fiber.reset();
}

void BindLazyRender(nb::module_& module)
{
    nb::class_<PyLazyRender>(module, "_LazyRender").def("_next", &PyLazyRender::Next).def("close", &PyLazyRender::Close);
    module.def("_lazy_generate_available", &LazyGenerateAvailable);
    module.def("_set_lazy_generate", [](bool enabled) { g_lazyEnabled = enabled; }, nb::arg("enabled"));
    // A generator closed or collected after this renders nothing more: its fiber is freed as it stands
    nb::module_::import_("atexit").attr("register")(nb::cpp_function([] { g_exiting = true; }));
}

nb::object MakeLazyRender(nb::object tpl, nb::args args, nb::kwargs kwargs)
{
    if (!LazyGenerateAvailable())
    {
        return nb::none();
    }
    try
    {
        return nb::cast(std::make_unique<PyLazyRender>(std::move(tpl), std::move(args), std::move(kwargs)).release(), nb::rv_policy::take_ownership);
    }
    catch (const std::bad_alloc&)
    {
        // No stack for a fiber: eager
        return nb::none();
    }
}
} // namespace jinja2cpp_py

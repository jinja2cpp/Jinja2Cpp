#include "generate.h"

#include "environment.h"
#include "errors.h"

#include <jinja2cpp/template.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
using namespace nb::literals;

namespace
{
// The output a worker gathers before it hands it to the consumer and waits: the template runs at most this far ahead
// of the items the consumer took (docs/tasks/0436)
constexpr std::size_t kBatch = 512;
// How often a consumer on the main thread that waits for the worker checks its signals (Ctrl-C)
constexpr std::chrono::milliseconds kSignalCheck{ 50 };

// Thrown on the worker by its sink or poll to end a render the consumer closed or interrupted
class RenderCancelled final : public std::exception
{
public:
    [[nodiscard]] const char* what() const noexcept override { return "the generate() render was closed"; }
};

// One generate() render. The worker (a Python daemon thread) and the consumer (the thread that calls next()) take
// turns: one runs while the other waits without the GIL, so the items, the error and the Python objects are only
// touched by the side whose turn it is, under the GIL. The engine's render keeps its stack on the worker between
// two items, which is what makes the render lazy without a coroutine in the engine
class LazyRender
{
public:
    LazyRender(nb::object tpl, nb::object args, nb::object kwargs)
        : m_template(std::move(tpl))
        , m_args(std::move(args))
        , m_kwargs(std::move(kwargs))
    {
    }

    // __next__ (GIL held)
    nb::object Next()
    {
        for (;;)
        {
            if (m_running.load())
            {
                PyErr_SetString(PyExc_ValueError, "generator already executing");
                throw nb::python_error();
            }
            if (!m_items.empty())
            {
                auto item = MakeStr(m_items.front());
                m_items.pop_front();
                return item;
            }
            if (m_done)
            {
                if (m_error.is_valid() && !m_error.is_none())
                {
                    auto error = std::exchange(m_error, nb::none());
                    PyErr_SetObject(reinterpret_cast<PyObject*>(Py_TYPE(error.ptr())), error.ptr());
                    throw nb::python_error();
                }
                throw nb::stop_iteration();
            }
            Resume();
        }
    }

    // close(): ends a render that has not finished, as closing Jinja2's generator does (GIL held)
    void Close()
    {
        if (m_running)
        {
            PyErr_SetString(PyExc_ValueError, "generator already executing");
            throw nb::python_error();
        }
        m_items.clear();
        if (m_started && !m_done)
        {
            m_cancel.store(true);
            Resume();
        }
        m_done = true;
        m_error = nb::none();
    }

    [[nodiscard]] bool Parked() const { return m_started && !m_done && !m_running; }

    // On the worker, under the GIL: the whole render
    void Run()
    {
        struct Sink final : jinja2::OutputSink
        {
            explicit Sink(LazyRender& render)
                : m_render(render)
            {
            }
            void Write(std::string_view text) override { m_render.Add(text); }
            LazyRender& m_render;
        };
        Sink sink(*this);
        {
            // Thread.start() returns to the consumer only once it gets the GIL, so the render starts once the
            // consumer waits for it (Wait), which keeps its signals checked
            const nb::gil_scoped_release release;
            std::unique_lock lock(m_mutex);
            m_turn.wait(lock, [this] { return m_consumerWaits; });
        }
        t_current = this;
        m_worker = PyThread_get_thread_ident();
        try
        {
            nb::cast<const PyTemplate&>(m_template).RenderToSink(sink, nb::borrow<nb::args>(m_args), nb::borrow<nb::kwargs>(m_kwargs));
        }
        catch (nb::python_error& error)
        {
            if (!m_cancel.load())
            {
                m_error = nb::borrow(error.value());
            }
        }
        catch (const std::bad_alloc&)
        {
            PyErr_NoMemory();
            nb::python_error error;
            m_error = nb::borrow(error.value());
        }
        catch (const std::exception& error)
        {
            if (!m_cancel.load())
            {
                m_error = Names().templateRuntimeError(MakeStr(error.what()));
            }
        }
        t_current = nullptr;
        // An interrupt the render ended before Python code ran on the worker is not raised there later
        PyThreadState_SetAsyncExc(m_worker, nullptr);
        m_items.resize(m_cancel.load() ? 0 : m_items.size());
        // The worker may still hold the render after its turn (its thread ends later, at exit perhaps after the
        // interpreter stopped running threads): it keeps no Python object past this point
        m_template = nb::none();
        m_args = nb::none();
        m_kwargs = nb::none();
        {
            const std::lock_guard lock(m_mutex);
            m_done = true;
            m_running.store(false);
        }
        m_turn.notify_all();
    }

    // The poll of a render on the worker (PollLazyRender)
    static void Poll()
    {
        auto* render = t_current;
        if (!render)
        {
            return;
        }
        if (render->m_wantGil.load())
        {
            // The consumer waits for the GIL to check its signals
            const nb::gil_scoped_release release;
            std::unique_lock lock(render->m_mutex);
            render->m_turn.wait(lock, [render] { return !render->m_wantGil.load(); });
        }
        if (render->m_cancel.load())
        {
            throw RenderCancelled();
        }
    }

private:
    // The sink's chunk, on the worker
    void Add(std::string_view text)
    {
        if (m_cancel.load())
        {
            throw RenderCancelled();
        }
        m_items.emplace_back(text);
        m_batched += text.size();
        if (m_batched >= kBatch)
        {
            m_batched = 0;
            HandOff();
        }
    }

    // The worker gives the turn to the consumer and waits for it to come back
    void HandOff()
    {
        {
            const nb::gil_scoped_release release;
            std::unique_lock lock(m_mutex);
            m_running.store(false);
            m_turn.notify_all();
            m_turn.wait(lock, [this] { return m_running.load(); });
        }
        if (m_cancel.load())
        {
            throw RenderCancelled();
        }
    }

    // The consumer gives the turn to the worker, starting it on the first call, and waits for the turn to come back
    void Resume()
    {
        {
            const std::lock_guard lock(m_mutex);
            m_running.store(true);
        }
        if (!m_started)
        {
            m_started = true;
            const auto threading = nb::module_::import_("threading");
            m_mainThread = threading.attr("current_thread")().is(threading.attr("main_thread")());
            // The render runs in a copy of the consumer's context (contextvars), as a Python callable it calls would
            // run in the consumer's own; a daemon thread, so an unfinished generator does not hold up the exit
            const auto context = nb::module_::import_("contextvars").attr("copy_context")();
            auto* self = this;
            auto body = nb::cpp_function([keep = m_self.lock(), self] { self->Run(); });
            threading.attr("Thread")("target"_a = context.attr("run"), "args"_a = nb::make_tuple(body), "name"_a = "jinja2cpp-generate", "daemon"_a = true).attr("start")();
        }
        else
        {
            m_turn.notify_all();
        }
        Wait();
    }

    // The consumer waits for the worker's turn to end; on the main thread it checks its signals meanwhile, and a
    // KeyboardInterrupt ends the render and is raised here
    void Wait()
    {
        nb::object interrupt;
        {
            const nb::gil_scoped_release release;
            std::unique_lock lock(m_mutex);
            if (!m_consumerWaits)
            {
                m_consumerWaits = true;
                m_turn.notify_all();
            }
            while (m_running.load())
            {
                if (m_turn.wait_for(lock, kSignalCheck, [this] { return !m_running.load(); }) || !m_mainThread || interrupt.is_valid())
                {
                    continue;
                }
                m_wantGil.store(true);
                lock.unlock();
                {
                    const nb::gil_scoped_acquire acquire;
                    if (PyErr_CheckSignals() != 0)
                    {
                        nb::python_error error;
                        interrupt = nb::borrow(error.value());
                        m_cancel.store(true);
                        // A Python callable of the render that runs on the worker stops too: the poll ends the
                        // render in the engine's code, KeyboardInterrupt in Python's
                        PyThreadState_SetAsyncExc(m_worker, PyExc_KeyboardInterrupt);
                    }
                }
                lock.lock();
                m_wantGil.store(false);
                m_turn.notify_all();
            }
        }
        if (interrupt.is_valid())
        {
            m_items.clear();
            m_error = nb::none();
            m_done = true;
            PyErr_SetObject(reinterpret_cast<PyObject*>(Py_TYPE(interrupt.ptr())), interrupt.ptr());
            throw nb::python_error();
        }
    }

    friend class Generation;

    static thread_local LazyRender* t_current;

    nb::object m_template;
    nb::object m_args;
    nb::object m_kwargs;
    // The items written and not yet taken, and the render's error; touched on the side whose turn it is
    std::deque<std::string> m_items;
    std::size_t m_batched = 0;
    nb::object m_error = nb::none();
    bool m_started = false;
    bool m_done = false;
    bool m_mainThread = false;
    // The worker's thread, for an interrupt (set before the render starts)
    unsigned long m_worker = 0;
    // The consumer waited for the worker's first turn: the worker starts the render. Under m_mutex
    bool m_consumerWaits = false;
    // Whose turn it is: the worker's while true. Changed under m_mutex
    std::atomic<bool> m_running{ false };
    std::atomic<bool> m_cancel{ false };
    std::atomic<bool> m_wantGil{ false };
    std::mutex m_mutex;
    std::condition_variable m_turn;
    std::weak_ptr<LazyRender> m_self;
};

thread_local LazyRender* LazyRender::t_current = nullptr;

// The renders whose worker is parked: closed before the interpreter finalizes, when no daemon thread can take
// the GIL back (under the GIL)
std::vector<std::weak_ptr<LazyRender>>& LiveRenders()
{
    static auto* renders = new std::vector<std::weak_ptr<LazyRender>>();
    return *renders;
}

// jinja2cpp._jinja2cpp._Generation: the iterator Template._generate returns
class Generation
{
public:
    Generation(nb::handle tpl, nb::tuple args, nb::dict kwargs)
        : m_render(std::make_shared<LazyRender>(nb::borrow(tpl), std::move(args), std::move(kwargs)))
    {
        nb::cast<const PyTemplate&>(tpl);
        m_render->m_self = m_render;
        auto& live = LiveRenders();
        live.erase(std::remove_if(live.begin(), live.end(), [](const auto& weak) { return weak.expired(); }), live.end());
        live.push_back(m_render);
    }
    ~Generation()
    {
        if (!m_render->Parked())
        {
            return;
        }
        try
        {
            // At finalization the worker could not take the GIL back: it stays parked, and the process ends it
            if (nb::cast<bool>(nb::module_::import_("sys").attr("is_finalizing")()))
            {
                return;
            }
            m_render->Close();
        }
        catch (nb::python_error& error)
        {
            error.discard_as_unraisable("closing a jinja2cpp generate()");
        }
    }
    Generation(const Generation&) = delete;
    Generation& operator=(const Generation&) = delete;
    Generation(Generation&&) = delete;
    Generation& operator=(Generation&&) = delete;

    nb::object Next() { return m_render->Next(); }
    void Close() { m_render->Close(); }

private:
    std::shared_ptr<LazyRender> m_render;
};

void CloseLiveRenders()
{
    for (const auto& weak : LiveRenders())
    {
        if (auto render = weak.lock(); render && render->Parked())
        {
            try
            {
                render->Close();
            }
            catch (nb::python_error& error)
            {
                error.discard_as_unraisable("closing a jinja2cpp generate() at exit");
            }
        }
    }
    LiveRenders().clear();
}
} // namespace

void PollLazyRender()
{
    LazyRender::Poll();
}

void BindGenerate(nb::module_& module)
{
    nb::class_<Generation>(module, "_Generation")
        .def(nb::init<nb::handle, nb::tuple, nb::dict>(), "template"_a, "args"_a, "kwargs"_a)
        .def("__iter__", [](nb::handle self) { return nb::borrow(self); })
        .def("__next__", &Generation::Next)
        .def("close", &Generation::Close);
    nb::module_::import_("atexit").attr("register")(nb::cpp_function(&CloseLiveRenders));
}
} // namespace jinja2cpp_py

#ifndef JINJA2CPP_PYTHON_COMMON_H
#define JINJA2CPP_PYTHON_COMMON_H

// The Python module jinja2cpp._jinja2cpp (docs/python.md, docs/tasks/0353). It links the library
// statically and builds on its private seam (src/host_object.h, TemplateImpl::RenderInternal).
// Every function here runs with the GIL held: a render holds it from start to end (0248 D5)

#include <nanobind/nanobind.h>

#include "internal_value.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2cpp_py
{
namespace nb = nanobind;

// A strong reference to a Python object that the engine owns: released with the GIL, which a
// value of the engine dropped outside a render may not hold (a thread-local cache at thread
// exit). nb::object would release it without the GIL
class PyRef
{
public:
    PyRef() noexcept = default;
    // Takes a new reference
    static PyRef Steal(PyObject* object) noexcept
    {
        PyRef ref;
        ref.m_object = object;
        return ref;
    }
    // Adds a reference
    static PyRef Borrow(PyObject* object) noexcept
    {
        Py_XINCREF(object);
        return Steal(object);
    }
    PyRef(const PyRef& other) noexcept
        : m_object(other.m_object)
    {
        // Copied by the engine's values inside a render, which holds the GIL
        Py_XINCREF(m_object);
    }
    PyRef(PyRef&& other) noexcept
        : m_object(std::exchange(other.m_object, nullptr))
    {
    }
    PyRef& operator=(PyRef other) noexcept
    {
        std::swap(m_object, other.m_object);
        return *this;
    }
    ~PyRef() { Release(m_object); }

    [[nodiscard]] PyObject* Get() const noexcept { return m_object; }
    explicit operator bool() const noexcept { return m_object != nullptr; }
    [[nodiscard]] nb::handle Handle() const noexcept { return m_object; }

    // Py_DECREF with the GIL: taken when this thread does not hold it, and not at all once the
    // interpreter is gone (the reference leaks then, as nanobind's own do)
    static void Release(PyObject* object) noexcept;

private:
    PyObject* m_object = nullptr;
};

// str(object) as UTF-8; throws nb::python_error. A lone surrogate raises UnicodeEncodeError, as
// Python's str.encode does
std::string Utf8(nb::handle text);
// The same for an object that is known to be a str
std::string_view Utf8View(nb::handle text);

// A Python str of UTF-8 text the engine made
nb::object MakeStr(std::string_view text);

// The objects of other modules the module uses, imported on first use: the module must not
// import jinja2cpp while jinja2cpp/__init__.py imports it
struct PyNames
{
    nb::object markupType;    // markupsafe.Markup
    nb::object undefinedType; // jinja2cpp.runtime.Undefined
    nb::object templateNotFound;
    nb::object templateSyntaxError;
    nb::object templateAssertionError;
    nb::object templateRuntimeError;
    nb::object undefinedError;
    nb::object securityError;
    nb::object templateError;
    nb::object reversed;
    nb::object dictType;
    nb::object mappingType; // collections.abc.Mapping
};
const PyNames& Names();
// Drops the references Names() holds, when the module is freed
void ClearNames() noexcept;

// Whether `type` is a jinja2 or jinja2cpp class named `name` (in jinja2.runtime, jinja2cpp.runtime
// and the like): real Jinja2's classes are accepted where ours are, without importing jinja2
bool IsJinjaClass(nb::handle type, std::string_view name);

} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_COMMON_H

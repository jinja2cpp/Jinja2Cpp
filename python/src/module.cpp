#include "common.h"
#include "context.h"
#include "convert.h"
#include "environment.h"
#include "errors.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include <limits>
#include <memory>
#include <string>
#include <string_view>

namespace jinja2cpp_py
{
using namespace nb::literals;

void PyRef::Release(PyObject* object) noexcept
{
    if (!object)
    {
        return;
    }
    if (HoldsGil())
    {
        Py_DECREF(object);
        return;
    }
    if (!Py_IsInitialized())
    {
        return;
    }
    // Safe whether or not this thread holds the GIL already
    const PyGILState_STATE state = PyGILState_Ensure();
    Py_DECREF(object);
    PyGILState_Release(state);
}

std::string_view Utf8View(nb::handle text)
{
    Py_ssize_t size = 0;
    const char* data = PyUnicode_AsUTF8AndSize(text.ptr(), &size);
    if (!data)
    {
        throw nb::python_error();
    }
    return { data, static_cast<size_t>(size) };
}

std::string Utf8(nb::handle text)
{
    if (PyUnicode_Check(text.ptr()))
    {
        return std::string(Utf8View(text));
    }
    const auto str = nb::steal(PyObject_Str(text.ptr()));
    if (!str)
    {
        throw nb::python_error();
    }
    return std::string(Utf8View(str));
}

nb::object MakeStr(std::string_view text)
{
    PyObject* str = PyUnicode_DecodeUTF8(text.data(), static_cast<Py_ssize_t>(text.size()), "strict");
    if (!str)
    {
        throw nb::python_error();
    }
    return nb::steal(str);
}

namespace
{
// Never freed: the interpreter may be gone when static objects are destroyed
PyNames* g_names = nullptr;
} // namespace

const PyNames& Names()
{
    if (!g_names)
    {
        auto names = std::make_unique<PyNames>();
        names->markupType = nb::module_::import_("markupsafe").attr("Markup");
        names->undefinedType = nb::module_::import_("jinja2cpp.runtime").attr("Undefined");
        const auto exceptions = nb::module_::import_("jinja2cpp.exceptions");
        names->templateNotFound = exceptions.attr("TemplateNotFound");
        names->templateSyntaxError = exceptions.attr("TemplateSyntaxError");
        names->templateAssertionError = exceptions.attr("TemplateAssertionError");
        names->templateRuntimeError = exceptions.attr("TemplateRuntimeError");
        names->undefinedError = exceptions.attr("UndefinedError");
        names->securityError = exceptions.attr("SecurityError");
        names->templateError = exceptions.attr("TemplateError");
        const auto builtins = nb::module_::import_("builtins");
        names->reversed = builtins.attr("reversed");
        names->dictType = builtins.attr("dict");
        names->mappingType = nb::module_::import_("collections.abc").attr("Mapping");
        g_names = names.release();
    }
    return *g_names;
}

void ClearNames() noexcept
{
    delete g_names;
    g_names = nullptr;
}

bool IsJinjaClass(nb::handle type, std::string_view name)
{
    if (!PyType_Check(type.ptr()))
    {
        return false;
    }
    PyObject* typeName = PyObject_GetAttrString(type.ptr(), "__name__");
    if (!typeName)
    {
        PyErr_Clear();
        return false;
    }
    const auto ownedName = nb::steal(typeName);
    if (!PyUnicode_Check(typeName) || Utf8View(ownedName) != name)
    {
        return false;
    }
    PyObject* module = PyObject_GetAttrString(type.ptr(), "__module__");
    if (!module)
    {
        PyErr_Clear();
        return false;
    }
    const auto ownedModule = nb::steal(module);
    if (!PyUnicode_Check(module))
    {
        return false;
    }
    const auto moduleName = Utf8View(ownedModule);
    return moduleName.substr(0, 6) == "jinja2" && (moduleName.size() == 6 || moduleName[6] == '.' || moduleName.substr(6, 4) == "cpp." || moduleName.substr(6) == "cpp");
}

namespace
{
int TemplateValueTraverse(PyObject* self, visitproc visit, void* arg)
{
    Py_VISIT(Py_TYPE(self));
    return 0;
}

int TemplateValueClear(PyObject* self)
{
    if (nb::inst_ready(self))
    {
        nb::inst_ptr<TemplateValue>(self)->Expire();
    }
    return 0;
}

PyType_Slot g_templateValueSlots[] = { { Py_tp_traverse, reinterpret_cast<void*>(&TemplateValueTraverse) }, { Py_tp_clear, reinterpret_cast<void*>(&TemplateValueClear) }, { 0, nullptr } };

// The sandbox's formatter asks before it builds a string of `size` characters (jinja2cpp.sandbox): past the cap of
// the running render (MaxSequenceSize, or Settings::Limits::maxSequenceSize) it raises what the engine's own
// str.format raises, a TemplateRuntimeError, before Python allocates anything
void CheckFormatSize(nb::handle size)
{
    unsigned long long count = PyLong_AsUnsignedLongLong(size.ptr());
    if (count == static_cast<unsigned long long>(-1) && PyErr_Occurred())
    {
        // Larger than 64 bits (or negative, which has no width): treated as the largest
        PyErr_Clear();
        count = std::numeric_limits<unsigned long long>::max();
    }
    CheckSequenceSize(count);
}
} // namespace
} // namespace jinja2cpp_py

NB_MODULE(_jinja2cpp, module) // NOLINT: nanobind's entry point
{
    using namespace jinja2cpp_py;
    module.doc() = "Jinja2C++ rendering Jinja2 templates (docs/python.md)";
    module.def("_check_sequence_size", &CheckFormatSize, "size"_a);
    BindEnvironment(module);
    BindTemplateModule(module);
    BindContext(module);
    nb::class_<TemplateValue>(module, "TemplateValue", nb::type_slots(g_templateValueSlots))
        .def("__call__", &TemplateValue::Call)
        .def("__getattr__", &TemplateValue::GetAttr)
        .def("__str__", &TemplateValue::Str)
        .def("__repr__", &TemplateValue::Repr);
}

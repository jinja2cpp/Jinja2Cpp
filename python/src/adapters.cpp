#include "adapters.h"

#include "context.h"
#include "convert.h"

#include "expression_evaluator.h"
#include "generic_adapters.h"
#include "host_object.h"
#include "internal_value.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
namespace
{
// Clears the pending Python error when it is one of `kinds` (Jinja2's "missing" errors) and
// says so; any other error stays pending
template<size_t N>
bool ClearIfMissing(PyObject* const (&kinds)[N])
{
    for (auto* kind : kinds)
    {
        if (PyErr_ExceptionMatches(kind))
        {
            PyErr_Clear();
            return true;
        }
    }
    return false;
}

bool ClearAttributeError()
{
    PyObject* const kinds[] = { PyExc_AttributeError };
    return ClearIfMissing(kinds);
}

// Jinja2's getitem/getattr fall back on AttributeError, LookupError and TypeError
bool ClearItemError()
{
    PyObject* const kinds[] = { PyExc_LookupError, PyExc_TypeError, PyExc_AttributeError };
    return ClearIfMissing(kinds);
}

bool ClearTypeError()
{
    PyObject* const kinds[] = { PyExc_TypeError };
    return ClearIfMissing(kinds);
}

[[noreturn]] void ThrowPending()
{
    throw nb::python_error();
}

// The items of a list or tuple by index, so the engine may read them more than once and knows
// their length (loop.length). The length is the live one: iteration stops at the end of a list
// the template shortens while it loops, and goes on into one it extends, as Python's list
// iterator does (IndexedListAccessorImpl's enumerator asks GetItemsCountImpl at every step)
class PySequenceItems final : public jinja2::IndexedListAccessorImpl<PySequenceItems>
{
public:
    PySequenceItems(PyRef sequence, bool isList)
        : m_sequence(std::move(sequence))
        , m_isList(isList)
    {
    }

    [[nodiscard]] size_t GetItemsCountImpl() const
    {
        return static_cast<size_t>(m_isList ? PyList_Size(m_sequence.Get()) : PyTuple_Size(m_sequence.Get()));
    }
    [[nodiscard]] std::optional<InternalValue> GetItem(int64_t idx) const override
    {
        PyObject* item = m_isList ? PyList_GetItem(m_sequence.Get(), static_cast<Py_ssize_t>(idx)) : PyTuple_GetItem(m_sequence.Get(), static_cast<Py_ssize_t>(idx));
        if (!item)
        {
            // An index past the end the template shortened the list to
            PyErr_Clear();
            return std::nullopt;
        }
        return ToEngine(item);
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return false; }
    // A list the template can change through its methods ({% do l.append(x) %}), so loop.length is
    // its len() when first read, as in Jinja2; a tuple's never changes
    [[nodiscard]] bool HasLiveSize() const override { return m_isList; }
    [[nodiscard]] jinja2::GenericList CreateGenericList() const override { return jinja2::GenericList(std::make_shared<const PySequenceItems>(*this)); }

private:
    PyRef m_sequence;
    bool m_isList;
};

nb::object Steal(PyObject* object)
{
    if (!object)
    {
        ThrowPending();
    }
    return nb::steal(object);
}

// The attributes of a builtin type whose instances have no __dict__ (dict, list, tuple): a name
// outside them is no attribute, which spares getattr's AttributeError on every `d.key`
const std::unordered_set<std::string>& TypeAttributes(PyObject* type)
{
    static std::unordered_set<std::string> dictNames;
    static std::unordered_set<std::string> listNames;
    static std::unordered_set<std::string> tupleNames;
    auto& names = type == reinterpret_cast<PyObject*>(&PyDict_Type) ? dictNames : type == reinterpret_cast<PyObject*>(&PyList_Type) ? listNames
                                                                                                                                    : tupleNames;
    if (names.empty())
    {
        const auto dir = Steal(PyObject_Dir(type));
        for (auto name : dir)
        {
            names.insert(Utf8(name));
        }
    }
    return names;
}

PyHost::Kind KindOf(PyObject* object)
{
    if (PyDict_CheckExact(object))
    {
        return PyHost::Kind::Dict;
    }
    if (PyList_CheckExact(object))
    {
        return PyHost::Kind::List;
    }
    if (PyTuple_CheckExact(object))
    {
        return PyHost::Kind::Tuple;
    }
    return PyHost::Kind::Other;
}

// A Python key for the engine's key: a str made directly, the rest converted
nb::object KeyToPython(const InternalValue& key)
{
    if (const auto text = jinja2::AsStringView<char>(key); text && !key.IsMarkup())
    {
        return MakeStr(*text);
    }
    return ToPython(key);
}

// The items of an iterator, read once
ListAdapter IteratorAdapter(nb::object iterator)
{
    auto ref = PyRef::Steal(iterator.release().ptr());
    return ListAdapter::CreateAdapter([ref = std::move(ref)]() -> std::optional<InternalValue> {
        PyObject* item = PyIter_Next(ref.Get());
        if (!item)
        {
            if (PyErr_Occurred())
            {
                ThrowPending();
            }
            return std::nullopt;
        }
        return ToEngine(nb::steal(item));
    });
}
} // namespace

PyHost::PyHost(nb::handle object, bool borrowed)
    : m_object(object.ptr())
    , m_borrowed(borrowed)
    , m_kind(KindOf(object.ptr()))
{
    if (!m_borrowed)
    {
        Py_INCREF(m_object);
    }
}

PyHost::~PyHost()
{
    if (!m_borrowed)
    {
        PyRef::Release(m_object);
    }
}

const char* PyHost::TypeName() const
{
    if (m_typeName.empty())
    {
        auto* type = reinterpret_cast<PyObject*>(Py_TYPE(m_object));
        const auto name = Steal(PyObject_GetAttrString(type, "__name__"));
        m_typeName = Utf8(name);
    }
    return m_typeName.c_str();
}

std::string PyHost::Str() const
{
    return Utf8(Steal(PyObject_Str(m_object)));
}

std::string PyHost::Repr() const
{
    return Utf8(Steal(PyObject_Repr(m_object)));
}

std::optional<InternalValue> PyHost::GetAttr(const std::string& name) const
{
    if (m_kind != Kind::Other)
    {
        const auto& names = TypeAttributes(reinterpret_cast<PyObject*>(Py_TYPE(m_object)));
        if (names.find(name) == names.end())
        {
            return std::nullopt;
        }
    }
    const auto pyName = MakeStr(name);
    PyObject* attr = PyObject_GetAttr(m_object, pyName.ptr());
    if (!attr)
    {
        if (ClearAttributeError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    const auto value = nb::steal(attr);
    if (const auto* render = CurrentRender(); render && render->sandbox)
    {
        // The one checkpoint of the sandbox (0248 D8): every attribute a template reads of a Python
        // object, whatever syntax reads it (x.a, x[a], |attr, attribute=, a str.format field), comes
        // here. The environment's _sandbox_attribute asks is_safe_attribute and gives the value, a
        // wrapped str.format, or the unsafe undefined
        const auto checked = Steal(PyObject_CallMethod(render->sandbox, "_sandbox_attribute", "OOO", m_object, pyName.ptr(), value.ptr()));
        return ToEngine(checked);
    }
    return ToEngine(value);
}

std::optional<InternalValue> PyHost::GetItem(const InternalValue& key) const
{
    const auto pyKey = KeyToPython(key);
    if (m_kind == Kind::Dict)
    {
        // Borrowed; no KeyError made for a missing key
        PyObject* item = PyDict_GetItemWithError(m_object, pyKey.ptr());
        if (item)
        {
            return ToEngine(item);
        }
        if (!PyErr_Occurred() || ClearItemError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    PyObject* item = PyObject_GetItem(m_object, pyKey.ptr());
    if (!item)
    {
        if (ClearItemError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    return ToEngine(nb::steal(item));
}

std::optional<InternalValue> PyHost::GetSlice(const InternalValue& start, const InternalValue& stop, const InternalValue& step) const
{
    if (m_kind == Kind::Other && !PySequence_Check(m_object) && !PyMapping_Check(m_object))
    {
        return std::nullopt;
    }
    // The engine passes None for an omitted bound; an undefined one stays an Undefined, which has no
    // __index__, so Python's slice raises TypeError as in Jinja2 (`l[missing:]`)
    const auto slice = Steal(PySlice_New(ToPython(start).ptr(), ToPython(stop).ptr(), ToPython(step).ptr()));
    return ToEngine(Steal(PyObject_GetItem(m_object, slice.ptr())));
}

bool PyHost::IsCallable() const
{
    return m_kind == Kind::Other && PyCallable_Check(m_object) != 0;
}

InternalValue PyHost::Call(const CallParams& params, RenderContext& context) const
{
    if (const auto* render = CurrentRender(); render && render->sandbox)
    {
        // SandboxedEnvironment.call: is_safe_callable, else a SecurityError
        const auto safe = Steal(PyObject_CallMethod(render->sandbox, "_sandbox_check_call", "O", m_object));
        (void)safe;
    }
    return CallPython(m_object, params, context);
}

bool PyHost::IsIterable() const
{
    if (m_kind != Kind::Other)
    {
        return true;
    }
    // iter(x) works: a fresh iterator of a container, or the iterator itself, is not read
    PyObject* iterator = PyObject_GetIter(m_object);
    if (!iterator)
    {
        if (ClearTypeError())
        {
            return false;
        }
        ThrowPending();
    }
    Py_DECREF(iterator);
    return true;
}

std::optional<ListAdapter> PyHost::Iterate() const
{
    if (m_kind == Kind::List || m_kind == Kind::Tuple)
    {
        return ListAdapter(jinja2::MakeRef<PySequenceItems>(PyRef::Borrow(m_object), m_kind == Kind::List));
    }
    PyObject* iterator = PyObject_GetIter(m_object);
    if (!iterator)
    {
        if (ClearTypeError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    return IteratorAdapter(nb::steal(iterator));
}

std::optional<ListAdapter> PyHost::Reversed() const
{
    PyObject* reversed = PyObject_CallFunctionObjArgs(Names().reversed.ptr(), m_object, nullptr);
    if (!reversed)
    {
        if (ClearTypeError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    return IteratorAdapter(nb::steal(reversed));
}

std::optional<bool> PyHost::Contains(const InternalValue& item) const
{
    const auto pyItem = KeyToPython(item);
    const int result = PySequence_Contains(m_object, pyItem.ptr());
    if (result < 0)
    {
        ThrowPending();
    }
    return result != 0;
}

std::optional<size_t> PyHost::Len() const
{
    const Py_ssize_t size = PyObject_Size(m_object);
    if (size < 0)
    {
        if (ClearTypeError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    return static_cast<size_t>(size);
}

bool PyHost::Truth() const
{
    const int result = PyObject_IsTrue(m_object);
    if (result < 0)
    {
        ThrowPending();
    }
    return result != 0;
}

std::optional<bool> PyHost::Equals(const InternalValue& other) const
{
    const auto pyOther = ToPython(other);
    // Python's ==: reflected operands and the identity fallback included
    const auto result = Steal(PyObject_RichCompare(m_object, pyOther.ptr(), Py_EQ));
    const int truth = PyObject_IsTrue(result.ptr());
    if (truth < 0)
    {
        ThrowPending();
    }
    return truth != 0;
}

std::optional<size_t> PyHost::Hash() const
{
    const Py_hash_t hash = PyObject_Hash(m_object);
    if (hash == -1 && PyErr_Occurred())
    {
        if (ClearTypeError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    return static_cast<size_t>(hash);
}

bool PyHost::IsMapping() const
{
    if (m_mapping < 0)
    {
        int mapping = m_kind == Kind::Dict ? 1 : 0;
        if (m_kind == Kind::Other)
        {
            mapping = PyObject_IsInstance(m_object, Names().mappingType.ptr());
            if (mapping < 0)
            {
                ThrowPending();
            }
        }
        m_mapping = mapping != 0 ? 1 : 0;
    }
    return m_mapping != 0;
}

bool PyHost::IsSequence() const
{
    return m_kind == Kind::List || m_kind == Kind::Tuple || PyList_Check(m_object) || PyTuple_Check(m_object);
}

bool PyHost::IsTuple() const
{
    return m_kind == Kind::Tuple || PyTuple_Check(m_object);
}

bool PyHost::IsMarkupLike() const
{
    if (m_markupLike < 0)
    {
        m_markupLike = m_kind == Kind::Other && PyObject_HasAttrString(m_object, "__html__") != 0 ? 1 : 0;
    }
    return m_markupLike != 0;
}

std::string PyHost::Html() const
{
    return Utf8(Steal(PyObject_CallMethod(m_object, "__html__", nullptr)));
}

namespace
{
nb::object Stub(const char* name)
{
    // A class of that name, for an object of the engine that has no Python class: loop, a macro, a cycler
    const auto type = Steal(PyObject_CallFunction(reinterpret_cast<PyObject*>(&PyType_Type), "s(O){}", name, reinterpret_cast<PyObject*>(&PyBaseObject_Type)));
    return Steal(PyObject_CallNoArgs(type.ptr()));
}

// A Python object of the kind of `obj`, for is_safe_attribute to look at: the value itself when it is
// small, an empty container otherwise (the checks look at the type; converting a list for each refused
// name would cost its length)
nb::object Representative(const InternalValue& obj, bool& synthetic)
{
    switch (obj.Kind())
    {
    case jinja2::ValueKind::None:
        return nb::none();
    case jinja2::ValueKind::Bool:
    case jinja2::ValueKind::Int:
    case jinja2::ValueKind::Double:
    case jinja2::ValueKind::BigInt:
    case jinja2::ValueKind::String:
        return ToPython(obj);
    case jinja2::ValueKind::List:
    {
        const auto list = *jinja2::AsList(obj);
        if (list.GetRangeInfo())
        {
            return Steal(PyObject_CallFunction(reinterpret_cast<PyObject*>(&PyRange_Type), "i", 0));
        }
        return list.IsTuple() ? Steal(PyTuple_New(0)) : Steal(PyList_New(0));
    }
    case jinja2::ValueKind::Map:
    {
        const auto map = *jinja2::AsMap(obj);
        if (const char* objectClass = map.GetObjectClass())
        {
            synthetic = true;
            return Stub(objectClass);
        }
        if (map.IsNamespace())
        {
            synthetic = true;
            return Stub("Namespace");
        }
        return nb::dict();
    }
    case jinja2::ValueKind::KeyValuePair:
        return Steal(PyTuple_New(0));
    case jinja2::ValueKind::Callable:
        synthetic = true;
        return Stub(jinja2::GetIf<jinja2::Callable>(&obj)->GetKind() == jinja2::Callable::Macro ? "Macro" : "function");
    default:
        synthetic = true;
        return Stub("object");
    }
}

// The ISandboxPolicy of a SandboxedEnvironment: the environment's own is_safe_attribute and wrap_str_format, asked
// for the engine's values. It borrows the environment, which holds the engine environment that holds this
class PySandboxPolicy final : public jinja2::ISandboxPolicy
{
public:
    PySandboxPolicy(PyObject* environment, bool allNames)
        : m_environment(environment)
        , m_allNames(allNames)
    {
    }

    [[nodiscard]] std::optional<InternalValue> NativeAttribute(const InternalValue& obj, const std::string& name) const override
    {
        if (name == "format_map" && jinja2::IsStringValue(obj))
        {
            // The engine's strings have no format_map; the sandbox's is SandboxedFormatter's
            return InternalValue(jinja2::Callable(jinja2::Callable::UserCallable, [obj, this](const CallParams& params, RenderContext& context) { return Format(obj, true, params, context); }));
        }
        // Python's objects have attributes the engine's values have not: __class__, __init__, ...
        // A public name is the environment's to judge only when its is_safe_attribute is not Jinja2's, which
        // refuses private names and the methods of lists and dicts that change them (the engine does the latter)
        const bool isPrivate = !name.empty() && name[0] == '_';
        if (!isPrivate && !m_allNames)
        {
            return std::nullopt;
        }
        bool synthetic = false;
        const auto representative = Representative(obj, synthetic);
        const auto pyName = MakeStr(name);
        nb::object owned = nb::none();
        // A real Python object has the attribute or has not. An object of the engine that is only
        // a stand-in (loop, a macro, self) is taken to have every name, private or public that a custom
        // is_safe_attribute judges: the refusal is the safe answer where Python's class might have it (its
        // mangled names). Its `value` is None: the engine does not build one for the question
        PyObject* value = synthetic ? nullptr : PyObject_GetAttr(representative.ptr(), pyName.ptr());
        if (!value && !synthetic)
        {
            if (ClearAttributeError())
            {
                return std::nullopt;
            }
            ThrowPending();
        }
        if (value)
        {
            owned = nb::steal(value);
        }
        const auto checked = Steal(PyObject_CallMethod(m_environment, "_sandbox_attribute", "OOO", representative.ptr(), pyName.ptr(), owned.ptr()));
        auto result = ToEngine(checked);
        if (result.IsUndefined())
        {
            return result;
        }
        return std::nullopt;
    }

    InternalValue Format(const InternalValue& self, bool formatMap, const CallParams& params, RenderContext& context) const override
    {
        const auto text = ToPython(self);
        const auto method = Steal(PyObject_GetAttrString(text.ptr(), formatMap ? "format_map" : "format"));
        auto wrapped = Steal(PyObject_CallMethod(m_environment, "wrap_str_format", "O", method.ptr()));
        if (wrapped.is_none())
        {
            wrapped = method;
        }
        return CallPython(wrapped.ptr(), params, context);
    }

private:
    PyObject* m_environment;
    bool m_allNames;
};
} // namespace

bool SandboxJudgesPublicNames(nb::handle environment)
{
    const auto all = nb::steal(PyObject_CallMethod(environment.ptr(), "_sandbox_judges_public_names", nullptr));
    if (!all)
    {
        throw nb::python_error();
    }
    return PyObject_IsTrue(all.ptr()) != 0;
}

std::shared_ptr<const jinja2::ISandboxPolicy> MakePythonSandboxPolicy(nb::handle environment, bool allNames)
{
    return std::make_shared<PySandboxPolicy>(environment.ptr(), allNames);
}

const PyHost* AsPyHost(const InternalValue& value)
{
    // The module makes every host object of the engine's values
    return static_cast<const PyHost*>(jinja2::GetHostObject(value));
}

InternalValue CallPython(PyObject* callable, const CallParams& params, RenderContext& context)
{
    const LeaseScope lease(context);
    // pass_context and its kin: the argument the callable asked for goes first
    const PassArg pass = PassArgOf(callable);
    const size_t extra = pass != PassArg::None ? 1 : 0;
    const size_t positional = extra + params.posParams.size();
    const size_t keywords = params.kwParams.size();
    // The arguments live in `objects`; `args` has a free first slot for the vectorcall protocol
    std::vector<nb::object> objects;
    objects.reserve(positional + keywords);
    std::vector<PyObject*> args(1 + positional + keywords, nullptr);
    if (extra != 0)
    {
        objects.push_back(MakePassArgument(pass, context));
        args[1] = objects.back().ptr();
    }
    for (size_t i = extra; i < positional; ++i)
    {
        objects.push_back(ToPython(params.posParams[i - extra]));
        args[1 + i] = objects.back().ptr();
    }
    nb::object names;
    if (keywords != 0)
    {
        names = Steal(PyTuple_New(static_cast<Py_ssize_t>(keywords)));
        size_t i = 0;
        for (const auto& [name, value] : params.kwParams)
        {
            auto pyName = MakeStr(name);
            // PyTuple_SetItem steals the reference
            PyTuple_SetItem(names.ptr(), static_cast<Py_ssize_t>(i), pyName.release().ptr());
            objects.push_back(ToPython(value));
            args[1 + positional + i] = objects.back().ptr();
            ++i;
        }
    }
    const auto result = Steal(PyObject_Vectorcall(callable, args.data() + 1, positional | PY_VECTORCALL_ARGUMENTS_OFFSET, names.ptr()));
    return ToEngine(result);
}

} // namespace jinja2cpp_py

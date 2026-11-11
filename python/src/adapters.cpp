#include "adapters.h"

#include "context.h"
#include "convert.h"
#include "errors.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
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
const std::set<std::string, std::less<>>& TypeAttributes(PyObject* type)
{
    static std::set<std::string, std::less<>> dictNames;
    static std::set<std::string, std::less<>> listNames;
    static std::set<std::string, std::less<>> tupleNames;
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
nb::object KeyToPython(const jinja2::Value& key)
{
    if (const auto* text = key.GetIf<std::string>(); text && !key.IsMarkup())
    {
        return MakeStr(*text);
    }
    return ToPython(key);
}

// The items of a Python iterator, read once: every enumerator of the list steps the one iterator, as
// a second loop over a generator in Jinja2 sees what the first left
class PyIteratorItems final : public jinja2::IListItemAccessor
{
public:
    explicit PyIteratorItems(PyRef iterator)
        : m_iterator(std::move(iterator))
    {
    }

    [[nodiscard]] const jinja2::IIndexBasedAccessor* GetIndexer() const override { return nullptr; }
    [[nodiscard]] std::optional<jinja2::ListEnumeratorPtr> CreateEnumerator() const override { return MakeEnumerator<Enumerator>(m_iterator); }
    [[nodiscard]] std::optional<size_t> GetSize() const override { return std::nullopt; }
    [[nodiscard]] bool IsEqual(const jinja2::IComparable& other) const override { return this == &other; }

private:
    class Enumerator final : public jinja2::IListEnumerator
    {
    public:
        explicit Enumerator(PyRef iterator)
            : m_iterator(std::move(iterator))
        {
        }

        void Reset() override {}
        bool MoveNext() override
        {
            PyObject* item = PyIter_Next(m_iterator.Get());
            if (!item)
            {
                if (PyErr_Occurred())
                {
                    ThrowPending();
                }
                m_current = jinja2::Value();
                return false;
            }
            m_current = ToEngine(nb::steal(item));
            return true;
        }
        [[nodiscard]] jinja2::Value GetCurrent() const override { return m_current; }
        [[nodiscard]] jinja2::ListEnumeratorPtr Clone() const override
        {
            auto clone = MakeEnumerator<Enumerator>(m_iterator);
            static_cast<Enumerator&>(*clone).m_current = m_current;
            return clone;
        }
        jinja2::ListEnumeratorPtr Move() override
        {
            auto moved = MakeEnumerator<Enumerator>(std::move(m_iterator));
            static_cast<Enumerator&>(*moved).m_current = std::move(m_current);
            return moved;
        }
        [[nodiscard]] bool IsEqual(const jinja2::IComparable& other) const override { return this == &other; }

    private:
        PyRef m_iterator;
        jinja2::Value m_current;
    };

    PyRef m_iterator;
};

jinja2::GenericList IteratorList(nb::object iterator)
{
    return jinja2::GenericList(std::make_shared<const PyIteratorItems>(PyRef::Steal(iterator.release().ptr())));
}

// Python's `object[key]` with Jinja2's missing errors cleared; null for a missing item
nb::object ItemOf(PyObject* object, PyHost::Kind kind, PyObject* key)
{
    if (kind == PyHost::Kind::Dict)
    {
        // Borrowed; no KeyError made for a missing key
        PyObject* item = PyDict_GetItemWithError(object, key);
        if (item)
        {
            return nb::borrow(item);
        }
        if (!PyErr_Occurred() || ClearItemError())
        {
            return {};
        }
        ThrowPending();
    }
    PyObject* item = PyObject_GetItem(object, key);
    if (!item)
    {
        if (ClearItemError())
        {
            return {};
        }
        ThrowPending();
    }
    return nb::steal(item);
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

nb::object PyHost::Attribute(std::string_view name) const
{
    if (m_kind != Kind::Other)
    {
        const auto& names = TypeAttributes(reinterpret_cast<PyObject*>(Py_TYPE(m_object)));
        if (names.find(name) == names.end())
        {
            return {};
        }
    }
    const auto pyName = MakeStr(name);
    PyObject* attr = PyObject_GetAttr(m_object, pyName.ptr());
    if (!attr)
    {
        if (ClearAttributeError())
        {
            return {};
        }
        ThrowPending();
    }
    auto value = nb::steal(attr);
    if (const auto* render = CurrentRender(); render && render->sandbox)
    {
        // The one checkpoint of the sandbox (0248 D8): every attribute a template reads of a Python
        // object, whatever syntax reads it (x.a, x[a], |attr, attribute=, a str.format field), comes
        // here. The environment's _sandbox_attribute asks is_safe_attribute and gives the value, a
        // wrapped str.format, or the unsafe undefined
        return Steal(PyObject_CallMethod(render->sandbox, "_sandbox_attribute", "OOO", m_object, pyName.ptr(), value.ptr()));
    }
    return value;
}

std::optional<jinja2::Value> PyHost::GetAttr(std::string_view name) const
{
    const auto value = Attribute(name);
    if (!value.is_valid())
    {
        return std::nullopt;
    }
    return ToEngine(value);
}

bool PyHost::GetAttrTo(std::string_view name, jinja2::HostResult& out) const
{
    const auto value = Attribute(name);
    if (!value.is_valid())
    {
        return false;
    }
    WriteToEngine(value, out);
    return true;
}

nb::object PyHost::ItemOfName(std::string_view key) const
{
    const auto pyKey = MakeStr(key);
    return ItemOf(m_object, m_kind, pyKey.ptr());
}

std::optional<jinja2::Value> PyHost::GetItem(const jinja2::Value& key) const
{
    const auto pyKey = KeyToPython(key);
    const auto item = ItemOf(m_object, m_kind, pyKey.ptr());
    if (!item.is_valid())
    {
        return std::nullopt;
    }
    return ToEngine(item);
}

bool PyHost::GetItemTo(std::string_view key, jinja2::HostResult& out) const
{
    const auto item = ItemOfName(key);
    if (!item.is_valid())
    {
        return false;
    }
    WriteToEngine(item, out);
    return true;
}

bool PyHost::ItemAtTo(size_t index, jinja2::HostResult& out) const
{
    const auto at = static_cast<Py_ssize_t>(index);
    PyObject* item = nullptr;
    if (m_kind == Kind::List)
    {
        item = PyList_GetItem(m_object, at);
    }
    else if (m_kind == Kind::Tuple)
    {
        item = PyTuple_GetItem(m_object, at);
    }
    else
    {
        // A list or tuple subclass: its own __getitem__
        PyObject* owned = PySequence_GetItem(m_object, at);
        if (!owned)
        {
            if (PyErr_ExceptionMatches(PyExc_IndexError))
            {
                PyErr_Clear();
                return false;
            }
            ThrowPending();
        }
        WriteToEngine(nb::steal(owned), out);
        return true;
    }
    if (!item)
    {
        // An index past the end the template shortened the list to
        PyErr_Clear();
        return false;
    }
    // A borrowed item: held while it is converted, which can run Python that drops it from the list
    WriteToEngine(nb::borrow(item), out);
    return true;
}

std::optional<jinja2::Value> PyHost::GetSlice(const jinja2::Value& start, const jinja2::Value& stop, const jinja2::Value& step) const
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

jinja2::Value PyHost::Call(const jinja2::UserCallableParams& params) const
{
    if (const auto* render = CurrentRender(); render && render->sandbox)
    {
        // SandboxedEnvironment.call: is_safe_callable, else a SecurityError
        const auto safe = Steal(PyObject_CallMethod(render->sandbox, "_sandbox_check_call", "O", m_object));
        (void)safe;
    }
    try
    {
        return CallPython(m_object, params);
    }
    catch (nb::python_error& error)
    {
        if (!error.matches(PyExc_StopIteration))
        {
            throw;
        }
    }
    // Jinja2's Context.call: a call that raises StopIteration gives the environment's undefined value
    const auto* render = CurrentRender();
    const nb::handle type = render && render->undefinedClass ? nb::handle(render->undefinedClass) : nb::handle(Names().undefinedType);
    return ToEngine(Steal(PyObject_CallFunction(type.ptr(), "s", "value was undefined because a callable raised a StopIteration exception")));
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

std::optional<jinja2::GenericList> PyHost::Iterate() const
{
    // An exact list or tuple is read by index (Len and ItemAtTo); the rest, a subclass and a dict's keys, through iter()
    PyObject* iterator = PyObject_GetIter(m_object);
    if (!iterator)
    {
        if (ClearTypeError())
        {
            return std::nullopt;
        }
        ThrowPending();
    }
    return IteratorList(nb::steal(iterator));
}

std::optional<jinja2::GenericList> PyHost::Reversed() const
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
    return IteratorList(nb::steal(reversed));
}

std::optional<bool> PyHost::Contains(const jinja2::Value& item) const
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
    if (m_kind == Kind::List)
    {
        return static_cast<size_t>(PyList_Size(m_object));
    }
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

std::optional<bool> PyHost::Equals(const jinja2::Value& other) const
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

// Only an exact list or tuple is read by index. A subclass is iterated through iter(), as Python iterates it: its own
// __iter__ when it overrides it, else the base type's iterator, which reads the items without the subclass's
// __getitem__
bool PyHost::IteratesByIndex() const
{
    return m_kind == Kind::List || m_kind == Kind::Tuple;
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

const PyHost* AsPyHost(const jinja2::Value& value)
{
    const auto* map = value.GetIf<jinja2::GenericMap>();
    if (!map || !map->GetAccessor())
    {
        return nullptr;
    }
    const auto* host = map->GetAccessor()->AsHostObject();
    // The module's objects are PyHost; an undefined value the module made (MakeUndefined) is another kind
    if (!host || typeid(*host) != typeid(PyHost))
    {
        return nullptr;
    }
    return static_cast<const PyHost*>(host);
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
// name would cost its length). A macro or `caller` reaches the policy as an empty value (sandbox_policy.h),
// which is None here: is_safe_attribute judges None's attributes for it
nb::object Representative(const jinja2::Value& obj, bool& synthetic)
{
    if (const auto* list = obj.GetIf<jinja2::GenericList>())
    {
        return list->IsTuple() ? Steal(PyTuple_New(0)) : Steal(PyList_New(0));
    }
    if (obj.GetIf<jinja2::ValuesList>())
    {
        return Steal(PyList_New(0));
    }
    if (const auto* map = obj.GetIf<jinja2::GenericMap>())
    {
        if (const char* objectClass = map->GetObjectClass())
        {
            synthetic = true;
            return Stub(objectClass);
        }
        return nb::dict();
    }
    if (obj.GetIf<jinja2::ValuesMap>())
    {
        return nb::dict();
    }
    if (obj.GetIf<jinja2::UserCallable>())
    {
        synthetic = true;
        return Stub("function");
    }
    // A long string, likewise, is an empty one of its type (str or Markup): a refused name read in a loop would
    // otherwise copy it into Python each time. A custom is_safe_attribute sees the value itself up to this length
    constexpr size_t longText = 4096;
    auto empty = std::visit(
        [](const auto& data) -> std::optional<jinja2::Value> {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view> || std::is_same_v<T, jinja2::Markup> || std::is_same_v<T, jinja2::WMarkup>)
            {
                if (data.size() > longText)
                {
                    return jinja2::Value(T{});
                }
            }
            return std::nullopt;
        },
        obj.GetData());
    return ToPython(empty ? *empty : obj);
}

// Whether `value` is a string of the template's, owned or viewed
bool IsText(const jinja2::Value& value)
{
    return std::visit(
        [](const auto& data) {
            using T = std::decay_t<decltype(data)>;
            return std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view> || std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view> || std::is_same_v<T, jinja2::Markup> || std::is_same_v<T, jinja2::WMarkup>;
        },
        value.GetData());
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

    [[nodiscard]] std::optional<jinja2::Value> NativeAttribute(const jinja2::Value& obj, std::string_view name) const override
    {
        if (name == "format_map" && IsText(obj))
        {
            // The engine's strings have no format_map; the sandbox's is SandboxedFormatter's
            // The callable may be kept past the render (a host argument is not lent): it refers to the environment
            // weakly, as a strong reference would make a cycle the garbage collector cannot see, and raises once the
            // environment is gone
            auto weak = PyRef::Steal(PyWeakref_NewRef(m_environment, nullptr));
            if (!weak)
            {
                throw nb::python_error();
            }
            jinja2::UserCallable formatMap([obj, weak = std::move(weak)](const jinja2::UserCallableParams& params) {
                // Calling the weak reference gives the environment or None (PyWeakref_GetRef is not in 3.12's stable ABI)
                const auto owner = PyRef::Steal(PyObject_CallNoArgs(weak.Get()));
                if (!owner.Get())
                {
                    throw nb::python_error();
                }
                if (owner.Get() == Py_None)
                {
                    RaiseTemplateRuntimeError("a value of the template was used after the call it was passed to ended");
                }
                return FormatWith(owner.Get(), obj, true, params);
            },
                                           { jinja2::ArgInfo(jinja2::ArgInfo::VarArgs), jinja2::ArgInfo(jinja2::ArgInfo::VarKwArgs) });
            formatMap.keepUndefined = true;
            return jinja2::Value(std::move(formatMap));
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
        if (jinja2::UndefinedOf(result))
        {
            return result;
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<jinja2::Value> Format(const jinja2::Value& self, bool formatMap, const jinja2::UserCallableParams& params) const override
    {
        return FormatWith(m_environment, self, formatMap, params);
    }

private:
    [[nodiscard]] static jinja2::Value FormatWith(PyObject* environment, const jinja2::Value& self, bool formatMap, const jinja2::UserCallableParams& params)
    {
        const auto text = ToPython(self);
        const auto method = Steal(PyObject_GetAttrString(text.ptr(), formatMap ? "format_map" : "format"));
        auto wrapped = Steal(PyObject_CallMethod(environment, "wrap_str_format", "O", method.ptr()));
        if (wrapped.is_none())
        {
            wrapped = method;
        }
        return CallPython(wrapped.ptr(), params);
    }

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

namespace
{
jinja2::Value CallPythonImpl(PyObject* callable, const jinja2::UserCallableParams& params)
{
    const LeaseScope lease(params);
    // pass_context and its kin: the argument the callable asked for goes first
    const PassArg pass = PassArgOf(callable);
    const size_t extra = pass != PassArg::None ? 1 : 0;
    const auto* posArgs = params.extraPosArgs.GetIf<jinja2::ValuesList>();
    const auto* kwArgs = params.extraKwArgs.GetIf<jinja2::ValuesMap>();
    const size_t positional = extra + (posArgs ? posArgs->size() : 0);
    const size_t keywords = kwArgs ? kwArgs->size() : 0;
    // The arguments live in `objects`; `args` has a free first slot for the vectorcall protocol
    std::vector<nb::object> objects;
    objects.reserve(positional + keywords);
    std::vector<PyObject*> args(1 + positional + keywords, nullptr);
    if (extra != 0)
    {
        objects.push_back(MakePassArgument(pass));
        args[1] = objects.back().ptr();
    }
    for (size_t i = extra; i < positional; ++i)
    {
        objects.push_back(ToPython((*posArgs)[i - extra]));
        args[1 + i] = objects.back().ptr();
    }
    nb::object names;
    if (keywords != 0)
    {
        names = Steal(PyTuple_New(static_cast<Py_ssize_t>(keywords)));
        size_t i = 0;
        for (const auto& [name, value] : *kwArgs)
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
} // namespace

jinja2::Value CallPython(PyObject* callable, const jinja2::UserCallableParams& params)
{
    try
    {
        return CallPythonImpl(callable, params);
    }
    catch (nb::python_error& error)
    {
        if (!error.matches(PyExc_Exception))
        {
            // KeyboardInterrupt, SystemExit: noted for the render, which raises it even where the engine absorbs
            // a failed call (RenderState::abort)
            if (auto* render = CurrentRender(); render && !render->abort)
            {
                // A copy: the error in flight may reach Python, which consumes it, before the render raises this one
                render->abort = std::make_exception_ptr(nb::python_error(error));
            }
        }
        throw;
    }
}

} // namespace jinja2cpp_py

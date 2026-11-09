#include "convert.h"

#include "adapters.h"
#include "errors.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2cpp_py
{
using jinja2::UndefinedDescription;
using jinja2::UndefinedPolicy;
using jinja2::Value;

namespace
{
thread_local RenderState* t_render = nullptr;
thread_local LeaseScope* t_lease = nullptr;

// The function of a callable the module made for a Python object (BorrowedCallable): it borrows the object
struct PyCallable
{
    PyObject* object;

    Value operator()(const jinja2::UserCallableParams& params) const { return CallPython(object, params); }
};

// Calls Py_EnterRecursiveCall for a nested conversion: a template can nest lists without nested syntax
// (a loop that wraps a list in a list), so the conversion stops with Python's RecursionError
class RecursionGuard
{
public:
    // Python's C recursion budget alone lets a deep conversion overflow the native stack where frames are large (a
    // sanitizer build), so the conversions also count their own depth against sys.getrecursionlimit()
    explicit RecursionGuard(const char* where)
    {
        if (t_depth >= Py_GetRecursionLimit())
        {
            PyErr_Format(PyExc_RecursionError, "maximum recursion depth exceeded%s", where);
            throw nb::python_error();
        }
        if (Py_EnterRecursiveCall(where) != 0)
        {
            throw nb::python_error();
        }
        ++t_depth;
    }
    ~RecursionGuard()
    {
        --t_depth;
        Py_LeaveRecursiveCall();
    }
    RecursionGuard(const RecursionGuard&) = delete;
    RecursionGuard& operator=(const RecursionGuard&) = delete;
    RecursionGuard(RecursionGuard&&) = delete;
    RecursionGuard& operator=(RecursionGuard&&) = delete;

private:
    static thread_local int t_depth;
};
thread_local int RecursionGuard::t_depth = 0;

Value IntToEngine(PyObject* number)
{
    int overflow = 0;
    const long long value = PyLong_AsLongLongAndOverflow(number, &overflow);
    if (overflow == 0)
    {
        if (value == -1 && PyErr_Occurred())
        {
            throw nb::python_error();
        }
        return Value(static_cast<int64_t>(value));
    }
    // Outside int64: read in hexadecimal, which sys.get_int_max_str_digits() does not limit
    const auto hex = nb::steal(PyNumber_ToBase(number, 16));
    if (!hex)
    {
        throw nb::python_error();
    }
    auto digits = Utf8View(hex);
    const bool negative = digits.front() == '-';
    digits.remove_prefix(negative ? 3 : 2);
    return Value(jinja2::BigInt::FromDigits(digits, 16, negative));
}

UndefinedPolicy PolicyOfClass(nb::handle type)
{
    // The nearest of Jinja2's four classes among the bases of `type`
    const auto mro = nb::steal(PyObject_GetAttrString(type.ptr(), "__mro__"));
    if (mro)
    {
        for (auto base : mro)
        {
            if (IsJinjaClass(base, "StrictUndefined"))
            {
                return UndefinedPolicy::Strict;
            }
            if (IsJinjaClass(base, "ChainableUndefined"))
            {
                return UndefinedPolicy::Chainable;
            }
            if (IsJinjaClass(base, "DebugUndefined"))
            {
                return UndefinedPolicy::Debug;
            }
            if (IsJinjaClass(base, "Undefined"))
            {
                return UndefinedPolicy::Default;
            }
        }
    }
    else
    {
        PyErr_Clear();
    }
    return UndefinedPolicy::Default;
}

// The answers of IsUndefinedType, per type. A static type lives as long as the interpreter; a
// heap type (a class of Python code, which may be made and dropped at run time) is held by a
// weak reference whose callback drops the entry, so the cache keeps no class alive
struct TypeAnswer
{
    bool undefined = false;
    PyRef weakref; // null for a static type
};
std::unordered_map<const PyTypeObject*, TypeAnswer>& KnownTypes()
{
    // Never freed: the interpreter may be gone when static objects are destroyed
    static auto* known = new std::unordered_map<const PyTypeObject*, TypeAnswer>();
    return *known;
}

void RememberType(PyTypeObject* type, bool undefined)
{
    auto* typeObject = reinterpret_cast<PyObject*>(type);
    TypeAnswer answer;
    answer.undefined = undefined;
    if ((PyType_GetFlags(type) & Py_TPFLAGS_HEAPTYPE) != 0)
    {
        const PyTypeObject* key = type;
        auto forget = nb::cpp_function([key](nb::handle) { KnownTypes().erase(key); });
        PyObject* weakref = PyWeakref_NewRef(typeObject, forget.ptr());
        if (!weakref)
        {
            // Not weakly referenceable: asked again each time rather than kept alive
            PyErr_Clear();
            return;
        }
        answer.weakref = PyRef::Steal(weakref);
    }
    KnownTypes().insert_or_assign(type, std::move(answer));
}

// Whether instances of `type` are Undefined, of jinja2cpp or of Jinja2 itself, found without
// importing jinja2: a base named Undefined from a jinja2 module. Asked for every Python object a
// template reads that is no dict, list, tuple or scalar, so the answer is kept per type
bool IsUndefinedType(PyTypeObject* type)
{
    const auto& known = KnownTypes();
    if (const auto found = known.find(type); found != known.end())
    {
        return found->second.undefined;
    }
    auto* typeObject = reinterpret_cast<PyObject*>(type);
    bool undefined = false;
    const int ours = PyObject_IsSubclass(typeObject, Names().undefinedType.ptr());
    if (ours < 0)
    {
        throw nb::python_error();
    }
    if (ours != 0)
    {
        undefined = true;
    }
    else if (const auto mro = nb::steal(PyObject_GetAttrString(typeObject, "__mro__")))
    {
        for (auto base : mro)
        {
            if (IsJinjaClass(base, "Undefined"))
            {
                undefined = true;
                break;
            }
        }
    }
    else
    {
        PyErr_Clear();
    }
    RememberType(type, undefined);
    return undefined;
}

Value UndefinedToEngine(PyObject* object)
{
    UndefinedDescription description;
    description.policy = PolicyOfClass(reinterpret_cast<PyObject*>(Py_TYPE(object)));
    const auto attr = [object](const char* name) -> nb::object {
        PyObject* value = PyObject_GetAttrString(object, name);
        if (!value)
        {
            PyErr_Clear();
            return nb::none();
        }
        return nb::steal(value);
    };
    if (const auto name = attr("_undefined_name"); PyUnicode_Check(name.ptr()))
    {
        description.name = Utf8(name);
    }
    if (const auto hint = attr("_undefined_hint"); PyUnicode_Check(hint.ptr()))
    {
        description.hint = Utf8(hint);
    }
    // Jinja2's unsafe_undefined: exc is a SecurityError class
    if (const auto exc = attr("_undefined_exception"); PyType_Check(exc.ptr()))
    {
        if (const auto mro = nb::steal(PyObject_GetAttrString(exc.ptr(), "__mro__")))
        {
            for (auto base : mro)
            {
                if (IsJinjaClass(base, "SecurityError"))
                {
                    description.security = true;
                    break;
                }
            }
        }
        else
        {
            PyErr_Clear();
        }
    }
    return jinja2::MakeUndefined(std::move(description));
}

// The engine's message of an undefined value that names an object (src/undefined.cpp, UndefinedMessage)
std::string UndefinedMessage(const UndefinedDescription& description)
{
    if (description.objType.empty())
    {
        return "'" + description.name + "' is undefined";
    }
    if (description.isAttr)
    {
        return "'" + description.objType + "' has no attribute '" + description.name + "'";
    }
    return description.objType + " has no element " + description.name;
}

nb::object UndefinedToPython(const UndefinedDescription& description)
{
    const auto* render = CurrentRender();
    const nb::handle type = render && render->undefinedClass ? nb::handle(render->undefinedClass) : nb::handle(Names().undefinedType);
    nb::dict kwargs;
    if (!description.hint.empty())
    {
        kwargs["hint"] = MakeStr(description.hint);
    }
    else if (!description.objType.empty())
    {
        // The missing attribute or item of an object Python cannot see: the message says it
        kwargs["hint"] = MakeStr(UndefinedMessage(description));
    }
    if (!description.name.empty())
    {
        kwargs["name"] = MakeStr(description.name);
    }
    if (description.security)
    {
        kwargs["exc"] = Names().securityError;
    }
    const auto noArgs = nb::steal(PyTuple_New(0));
    PyObject* result = PyObject_Call(type.ptr(), noArgs.ptr(), kwargs.ptr());
    if (!result)
    {
        throw nb::python_error();
    }
    return nb::steal(result);
}

// Containers seen while converting one value: a list that contains itself becomes a Python
// list that contains itself
using Memo = std::unordered_map<const void*, nb::object>;

nb::object ToPythonImpl(const Value& value, Memo& memo);

nb::object WrapTemplateValue(const Value& value)
{
    return nb::cast(new TemplateValue(value), nb::rv_policy::take_ownership);
}

nb::object WideToPython(std::wstring_view text)
{
    return MakeStr(jinja2::ConvertString<std::string>(text));
}

nb::object MarkupOf(nb::object text)
{
    return Names().markupType(text);
}

nb::object BigIntToPython(const jinja2::BigInt& big)
{
    // The magnitude in hexadecimal, which sys.get_int_max_str_digits() does not limit
    const auto hex = big.ToDigits(16);
    auto result = nb::steal(PyLong_FromString(hex.c_str(), nullptr, 16));
    if (!result)
    {
        throw nb::python_error();
    }
    if (big.Sign() < 0)
    {
        result = nb::steal(PyNumber_Negative(result.ptr()));
    }
    return result;
}

template<typename List>
nb::object ItemsToPython(const List& list, bool isTuple, const void* identity, Memo& memo)
{
    // A list of the engine may be lazy (range(10**12)): its items count against the sequence cap before they
    // are made, as the engine's own conversions count them
    std::size_t count = 0;
    if constexpr (std::is_same_v<List, jinja2::GenericList>)
    {
        if (const auto size = list.GetSize())
        {
            CheckSequenceSize(*size);
        }
    }
    if (identity)
    {
        if (auto found = memo.find(identity); found != memo.end())
        {
            return found->second;
        }
    }
    nb::list items;
    if (identity)
    {
        memo.emplace(identity, items);
    }
    for (const auto& item : list)
    {
        CheckSequenceSize(++count);
        items.append(ToPythonImpl(item, memo));
    }
    if (isTuple)
    {
        auto tuple = nb::steal(PySequence_Tuple(items.ptr()));
        if (identity)
        {
            memo[identity] = tuple;
        }
        return tuple;
    }
    return items;
}

nb::object MapToPython(const Value& value, const jinja2::GenericMap& map, Memo& memo)
{
    if (const auto* host = AsPyHost(value))
    {
        return nb::borrow(host->Object());
    }
    if (auto undefined = jinja2::UndefinedOf(value))
    {
        return UndefinedToPython(*undefined);
    }
    // loop, a cycler, a namespace, a module: objects that are no dict in Python either
    if (map.GetObjectClass() || (map.GetAccessor() && map.GetAccessor()->AsHostObject()))
    {
        return WrapTemplateValue(value);
    }
    const void* identity = map.GetIdentity();
    if (auto found = memo.find(identity); found != memo.end())
    {
        return found->second;
    }
    nb::dict items;
    memo.emplace(identity, items);
    // The entries with their own keys: `{1: 'a', '1': 'b'}` stays two entries
    for (const auto& [key, item] : map.GetItems())
    {
        auto pyKey = ToPythonImpl(key, memo);
        if (PyDict_SetItem(items.ptr(), pyKey.ptr(), ToPythonImpl(item, memo).ptr()) != 0)
        {
            throw nb::python_error();
        }
    }
    return items;
}

nb::object ToPythonImpl(const Value& value, Memo& memo)
{
    const RecursionGuard guard(" while converting a template value");
    return std::visit(
        [&](const auto& data) -> nb::object {
            using T = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<T, jinja2::NoneValue>)
            {
                return nb::none();
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return nb::bool_(data);
            }
            else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::string_view>)
            {
                return MakeStr(data);
            }
            else if constexpr (std::is_same_v<T, std::wstring> || std::is_same_v<T, std::wstring_view>)
            {
                return WideToPython(data);
            }
            else if constexpr (std::is_same_v<T, jinja2::Markup>)
            {
                return MarkupOf(MakeStr(data));
            }
            else if constexpr (std::is_same_v<T, jinja2::WMarkup>)
            {
                return MarkupOf(WideToPython(data));
            }
            else if constexpr (std::is_same_v<T, int64_t>)
            {
                return nb::steal(PyLong_FromLongLong(data));
            }
            else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, float>)
            {
                return nb::steal(PyFloat_FromDouble(static_cast<double>(data)));
            }
            else if constexpr (std::is_same_v<T, jinja2::BigInt>)
            {
                return BigIntToPython(data);
            }
            else if constexpr (std::is_same_v<T, jinja2::RecWrapper<jinja2::ValuesList>>)
            {
                return ItemsToPython(*data, false, nullptr, memo);
            }
            else if constexpr (std::is_same_v<T, jinja2::RecWrapper<jinja2::ValuesMap>>)
            {
                nb::dict items;
                for (const auto& [key, item] : *data)
                {
                    items[MakeStr(key)] = ToPythonImpl(item, memo);
                }
                return items;
            }
            else if constexpr (std::is_same_v<T, jinja2::GenericList>)
            {
                if (!data.IsValid())
                {
                    return nb::list();
                }
                return ItemsToPython(data, data.IsTuple(), data.GetIdentity(), memo);
            }
            else if constexpr (std::is_same_v<T, jinja2::GenericMap>)
            {
                return MapToPython(value, data, memo);
            }
            else if constexpr (std::is_same_v<T, jinja2::RecWrapper<jinja2::UserCallable>>)
            {
                if (const auto* own = data->callable.template target<PyCallable>())
                {
                    return nb::borrow(own->object);
                }
                return WrapTemplateValue(value);
            }
            else
            {
                return nb::none();
            }
        },
        value.GetData());
}

// The arguments of a call from Python, as a callable declared with ArgInfo::VarArgs and ArgInfo::VarKwArgs gets them
jinja2::UserCallableParams ToCallParams(nb::args args, nb::kwargs kwargs)
{
    jinja2::ValuesList positional;
    positional.reserve(args.size());
    for (auto arg : args)
    {
        positional.push_back(ToEngine(arg));
    }
    jinja2::ValuesMap keywords;
    for (auto [name, arg] : kwargs)
    {
        keywords[Utf8(name)] = ToEngine(arg);
    }
    jinja2::UserCallableParams params;
    params.extraPosArgs = Value(std::move(positional));
    params.extraKwArgs = Value(std::move(keywords));
    params.paramsParsed = true;
    return params;
}

// The parameters of a callable that names its arguments (a callable of the engine's own, ConfigureForChatTemplates'
// raise_exception): the positional ones fill its names in order, the rest by name
jinja2::UserCallableParams MapToArgsInfo(const jinja2::UserCallable& callable, jinja2::UserCallableParams params)
{
    bool varArgs = false;
    bool varKwArgs = false;
    for (const auto& info : callable.argsInfo)
    {
        varArgs = varArgs || info.paramName == jinja2::ArgInfo::VarArgs;
        varKwArgs = varKwArgs || info.paramName == jinja2::ArgInfo::VarKwArgs;
    }
    if (varArgs && varKwArgs)
    {
        return params;
    }
    auto& positional = params.extraPosArgs.AsList();
    auto& keywords = params.extraKwArgs.AsMap();
    size_t next = 0;
    for (const auto& info : callable.argsInfo)
    {
        if (!info.paramName.empty() && info.paramName[0] == '*')
        {
            continue;
        }
        if (auto found = keywords.find(info.paramName); found != keywords.end())
        {
            params.args[info.paramName] = std::move(found->second);
            keywords.erase(found);
        }
        else if (next < positional.size())
        {
            params.args[info.paramName] = std::move(positional[next++]);
        }
        else if (info.isMandatory)
        {
            params.paramsParsed = false;
        }
        else
        {
            params.args[info.paramName] = info.defValue;
        }
    }
    positional.erase(positional.begin(), positional.begin() + static_cast<std::ptrdiff_t>(next));
    return params;
}

// Python's text of an engine value, as `{{ value }}` prints it outside autoescape
std::string TextOf(const Value& value)
{
    if (const auto* host = AsPyHost(value))
    {
        return host->Str();
    }
    if (const auto* map = value.GetIf<jinja2::GenericMap>(); map && map->GetAccessor() && map->GetAccessor()->AsHostObject())
    {
        return map->GetAccessor()->AsHostObject()->Str();
    }
    return value.ToString();
}
} // namespace

RenderState* CurrentRender() noexcept
{
    return t_render;
}

PyObject* SandboxOf(nb::handle environment)
{
    PyObject* flag = PyObject_GetAttrString(environment.ptr(), "sandboxed");
    if (!flag)
    {
        PyErr_Clear();
        return nullptr;
    }
    const auto owned = nb::steal(flag);
    const int truth = PyObject_IsTrue(flag);
    if (truth < 0)
    {
        throw nb::python_error();
    }
    return truth != 0 ? environment.ptr() : nullptr;
}

void InitRenderState(RenderState& state, nb::handle environment)
{
    state.environment = environment.ptr();
    state.sandbox = SandboxOf(environment);
}

RenderScope::RenderScope(RenderState& state) noexcept
    : m_state(state)
{
    state.outer = t_render;
    t_render = &state;
}

RenderScope::~RenderScope()
{
    t_render = m_state.outer;
}

bool HoldsGil() noexcept
{
    // A render holds the GIL from start to end, and drops most of the references
    return t_render != nullptr;
}

LeaseScope::LeaseScope(const jinja2::UserCallableParams& params) noexcept
    : m_params(params)
    , m_outer(t_lease)
{
    t_lease = this;
}

LeaseScope::~LeaseScope()
{
    t_lease = m_outer;
    if (m_lease)
    {
        // The context refers to the render, which may end now
        m_lease->active = false;
        m_lease->context = Value();
    }
}

std::shared_ptr<CallLease> LeaseScope::Current()
{
    auto* scope = t_lease;
    if (!scope)
    {
        return nullptr;
    }
    if (!scope->m_lease)
    {
        auto lease = std::make_shared<CallLease>();
        lease->context = scope->m_params.context;
        lease->templateName = std::string(scope->m_params.site.templateName);
        lease->autoescape = scope->m_params.site.autoescape;
        scope->m_lease = std::move(lease);
    }
    return scope->m_lease;
}

void CheckNativeSandboxHooks(PyObject* sandbox)
{
    const auto checked = nb::steal(PyObject_CallMethod(sandbox, "_check_native_hooks", nullptr));
    if (!checked)
    {
        throw nb::python_error();
    }
}

TemplateValue::TemplateValue(jinja2::Value value)
    : m_value(std::move(value))
{
}

void TemplateValue::Expire() noexcept
{
    m_alive = false;
    m_value = jinja2::Value();
}

const jinja2::Value& TemplateValue::Value() const
{
    if (!m_alive)
    {
        RaiseTemplateRuntimeError("a value of the template was used after the call it was passed to ended");
    }
    return m_value;
}

void CheckSequenceSize(unsigned long long count)
{
    // The engine's cap and message for a sequence too long (Settings::Limits::maxSequenceSize)
    constexpr unsigned long long builtIn = 1ULL << 26;
    const auto* render = CurrentRender();
    const unsigned long long cap = render && render->maxSequenceSize != 0 ? render->maxSequenceSize : builtIn;
    if (count <= cap)
    {
        return;
    }
    const std::string length = "a list of length " + std::to_string(count);
    const auto message = count > builtIn && cap == builtIn ? length + " is too long" : length + " is longer than " + std::to_string(cap);
    RaiseTemplateRuntimeError(message);
}

namespace
{
// A call of the template's from Python failed: a KeyboardInterrupt or SystemExit a Python callable raised inside it is
// raised to the Python caller, which may catch it as in Jinja2, and is no longer the render's to raise (RenderState::abort)
[[noreturn]] void RaiseFailedCall()
{
    if (auto* render = CurrentRender(); render && render->abort)
    {
        std::rethrow_exception(std::exchange(render->abort, nullptr));
    }
    try
    {
        throw;
    }
    catch (const nb::python_error&)
    {
        throw;
    }
    catch (...)
    {
        RaiseCurrentEngineError();
    }
}
} // namespace

nb::object TemplateValue::Call(nb::args args, nb::kwargs kwargs) const
{
    if (const auto* render = CurrentRender(); render && render->sandbox)
    {
        // A macro or caller a Python callable got runs template code the environment's hooks may have been
        // overridden for since the template loaded
        CheckNativeSandboxHooks(render->sandbox);
    }
    const auto& value = Value();
    const auto* callable = value.GetIf<jinja2::UserCallable>();
    if (!callable)
    {
        if (const auto* map = value.GetIf<jinja2::GenericMap>(); map && map->GetAccessor())
        {
            if (const auto* host = map->GetAccessor()->AsHostObject(); host && host->IsCallable())
            {
                const auto result = [&] {
                    try
                    {
                        return host->Call(ToCallParams(std::move(args), std::move(kwargs)));
                    }
                    catch (...)
                    {
                        RaiseFailedCall();
                    }
                }();
                return ToPython(result);
            }
        }
        RaiseTypeError("'" + Repr() + "' object is not callable");
    }
    jinja2::Value result;
    try
    {
        result = callable->callable(MapToArgsInfo(*callable, ToCallParams(std::move(args), std::move(kwargs))));
    }
    catch (...)
    {
        RaiseFailedCall();
    }
    return ToPython(result);
}

nb::object TemplateValue::GetAttr(const std::string& name) const
{
    const auto& value = Value();
    std::optional<jinja2::Value> result;
    if (const auto* map = value.GetIf<jinja2::GenericMap>())
    {
        try
        {
            result = map->Find(name);
        }
        catch (const nb::python_error&)
        {
            throw;
        }
        catch (...)
        {
            RaiseCurrentEngineError();
        }
    }
    if (!result || jinja2::UndefinedOf(*result))
    {
        PyErr_Format(PyExc_AttributeError, "'%s' object has no attribute '%s'", TypeName().c_str(), name.c_str());
        throw nb::python_error();
    }
    return ToPython(*result);
}

std::string TemplateValue::TypeName() const
{
    if (!m_alive)
    {
        return "TemplateValue";
    }
    if (const auto* map = m_value.GetIf<jinja2::GenericMap>())
    {
        if (const char* objectClass = map->GetObjectClass())
        {
            return objectClass;
        }
        if (const auto* accessor = map->GetAccessor(); accessor && accessor->AsHostObject())
        {
            return accessor->AsHostObject()->TypeName();
        }
        return "dict";
    }
    if (m_value.GetIf<jinja2::UserCallable>())
    {
        return "Macro";
    }
    return "object";
}

std::string TemplateValue::Str() const
{
    // LoopContext and Namespace define only __repr__ in Jinja2, so str() is the repr
    if (const auto* map = m_alive ? m_value.GetIf<jinja2::GenericMap>() : nullptr; map && map->GetObjectClass())
    {
        return Repr();
    }
    return TextOf(Value());
}

std::string TemplateValue::Repr() const
{
    if (!m_alive)
    {
        return "<expired template value>";
    }
    if (const auto* map = m_value.GetIf<jinja2::GenericMap>(); map && map->GetAccessor() && map->GetAccessor()->AsHostObject())
    {
        return map->GetAccessor()->AsHostObject()->Repr();
    }
    if (m_value.GetIf<jinja2::UserCallable>())
    {
        return "<Macro>";
    }
    if (const auto* map = m_value.GetIf<jinja2::GenericMap>(); map && std::string_view(map->GetObjectClass() ? map->GetObjectClass() : "") == "LoopContext")
    {
        // Jinja2's LoopContext.__repr__: <LoopContext index/length>
        return "<LoopContext " + map->GetValueByName("index").ToString() + "/" + map->GetValueByName("length").ToString() + ">";
    }
    return "<" + TypeName() + " " + m_value.ToString() + ">";
}

namespace
{
// The common part of ToEngine and WriteToEngine: what `object` is to the engine, written by `sink`
// (Value(...) for ToEngine, HostResult::Set for WriteToEngine)
template<typename Sink>
auto Classify(nb::handle handle, Sink&& sink)
{
    PyObject* object = handle.ptr();
    if (object == Py_None)
    {
        return sink.None();
    }
    if (object == Py_True || object == Py_False)
    {
        return sink.Bool(object == Py_True);
    }
    if (PyUnicode_CheckExact(object))
    {
        return sink.Text(Utf8View(handle), false);
    }
    if (PyLong_CheckExact(object))
    {
        return sink.Int(object);
    }
    if (PyFloat_CheckExact(object))
    {
        return sink.Double(PyFloat_AsDouble(object));
    }
    // The common containers first: no question of their type
    if (PyDict_CheckExact(object) || PyList_CheckExact(object) || PyTuple_CheckExact(object))
    {
        return sink.Host(handle);
    }
    if (PyUnicode_Check(object))
    {
        // Markup is a str subclass: checked before the other subclasses, which are text
        const int markup = PyObject_IsInstance(object, Names().markupType.ptr());
        if (markup < 0)
        {
            throw nb::python_error();
        }
        return sink.Text(Utf8View(handle), markup != 0);
    }
    if (PyLong_Check(object))
    {
        return sink.Int(object);
    }
    if (PyFloat_Check(object))
    {
        const double value = PyFloat_AsDouble(object);
        if (value == -1.0 && PyErr_Occurred())
        {
            throw nb::python_error();
        }
        return sink.Double(value);
    }
    if (nb::isinstance<TemplateValue>(handle))
    {
        return sink.Engine(nb::cast<const TemplateValue&>(handle).Value());
    }
    if (IsUndefinedType(Py_TYPE(object)))
    {
        return sink.Engine(UndefinedToEngine(object));
    }
    return sink.Host(handle);
}

struct ValueSink
{
    static Value None() { return Value(); }
    static Value Bool(bool value) { return Value(value); }
    static Value Text(std::string_view text, bool markup) { return markup ? Value(jinja2::Markup(std::string(text))) : Value(std::string(text)); }
    static Value Int(PyObject* number) { return IntToEngine(number); }
    static Value Double(double value) { return Value(value); }
    static Value Host(nb::handle object) { return jinja2::MakeHostValue<PyHost>(object, false); }
    static Value Engine(Value value) { return value; }
};

struct ResultSink
{
    jinja2::HostResult& out;

    void None() const { out.SetNone(); }
    void Bool(bool value) const { out.Set(value); }
    void Text(std::string_view text, bool markup) const { markup ? out.SetMarkup(text) : out.SetString(text); }
    void Int(PyObject* number) const
    {
        int overflow = 0;
        const long long value = PyLong_AsLongLongAndOverflow(number, &overflow);
        if (overflow == 0)
        {
            if (value == -1 && PyErr_Occurred())
            {
                throw nb::python_error();
            }
            out.Set(static_cast<int64_t>(value));
            return;
        }
        out.Set(IntToEngine(number));
    }
    void Double(double value) const { out.Set(value); }
    void Host(nb::handle object) const { out.SetHostObject<PyHost>(object, false); }
    void Engine(Value value) const { out.Set(std::move(value)); }
};
} // namespace

Value ToEngine(nb::handle handle)
{
    return Classify(handle, ValueSink{});
}

void WriteToEngine(nb::handle handle, jinja2::HostResult& out)
{
    Classify(handle, ResultSink{ out });
}

nb::object ToPython(const Value& value)
{
    Memo memo;
    return ToPythonImpl(value, memo);
}

jinja2::UserCallable BorrowedCallable(nb::handle object)
{
    jinja2::UserCallable callable(PyCallable{ object.ptr() }, { jinja2::ArgInfo(jinja2::ArgInfo::VarArgs), jinja2::ArgInfo(jinja2::ArgInfo::VarKwArgs), jinja2::ArgInfo(jinja2::ArgInfo::Context) });
    callable.keepUndefined = true;
    return callable;
}

} // namespace jinja2cpp_py

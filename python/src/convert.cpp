#include "convert.h"

#include "adapters.h"
#include "errors.h"

#include "big_int.h"
#include "cycle_collector.h"
#include "expression_evaluator.h"
#include "host_object.h"
#include "internal_value.h"
#include "out_stream.h"
#include "render_context.h"
#include "undefined.h"
#include "value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
using jinja2::Callable;
using jinja2::UndefinedInfo;
using jinja2::UndefinedPolicy;
using jinja2::ValueKind;

namespace
{
thread_local RenderState* t_render = nullptr;
thread_local LeaseScope* t_lease = nullptr;

InternalValue StrToEngine(PyObject* text, bool markup)
{
    const auto view = Utf8View(text);
    auto value = InternalValue::FromText<char>({ view });
    if (markup)
    {
        value.SetMarkup();
    }
    return value;
}

InternalValue IntToEngine(PyObject* number)
{
    int overflow = 0;
    const long long value = PyLong_AsLongLongAndOverflow(number, &overflow);
    if (overflow == 0)
    {
        if (value == -1 && PyErr_Occurred())
        {
            throw nb::python_error();
        }
        return InternalValue(static_cast<int64_t>(value));
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
    return jinja2::IntegerFromDigits(digits, 16, negative);
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

// Whether instances of `type` are Undefined, of jinja2cpp or of Jinja2 itself, found without
// importing jinja2: a base named Undefined from a jinja2 module. Asked for every Python object a
// template reads that is no dict, list, tuple or scalar, so the answer is kept per type; the
// cache holds a reference to each type, so that no other type takes its address
bool IsUndefinedType(PyTypeObject* type)
{
    static std::unordered_map<const PyTypeObject*, bool> known;
    if (const auto found = known.find(type); found != known.end())
    {
        return found->second;
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
    Py_INCREF(typeObject);
    known.emplace(type, undefined);
    return undefined;
}

InternalValue UndefinedToEngine(PyObject* object)
{
    UndefinedInfo info;
    info.policy = PolicyOfClass(reinterpret_cast<PyObject*>(Py_TYPE(object)));
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
        info.name = Utf8(name);
    }
    if (const auto hint = attr("_undefined_hint"); PyUnicode_Check(hint.ptr()))
    {
        info.hint = Utf8(hint);
    }
    const auto shared = jinja2::MakeRef<const UndefinedInfo>(std::move(info));
    return InternalValue(jinja2::UndefinedValue{ shared.get() });
}

nb::object UndefinedToPython(const InternalValue& value)
{
    const auto* render = CurrentRender();
    const nb::handle type = render && render->undefinedClass ? nb::handle(render->undefinedClass) : nb::handle(Names().undefinedType);
    const auto* info = jinja2::GetUndefinedInfo(value);
    nb::dict kwargs;
    if (info)
    {
        if (!info->hint.empty())
        {
            kwargs["hint"] = MakeStr(info->hint);
        }
        else if (!info->objType.empty())
        {
            // The missing attribute or item of an object Python cannot see: the message says it
            kwargs["hint"] = MakeStr(jinja2::UndefinedMessage(*info));
        }
        if (!info->name.empty())
        {
            kwargs["name"] = MakeStr(info->name);
        }
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

nb::object ToPythonImpl(const InternalValue& value, Memo& memo);

nb::object WrapTemplateValue(const InternalValue& value)
{
    return nb::cast(new TemplateValue(value, LeaseScope::Current()), nb::rv_policy::take_ownership);
}

nb::object ListToPython(const InternalValue& value, Memo& memo)
{
    const auto list = *jinja2::AsList(value);
    if (const auto* range = list.GetRangeInfo())
    {
        return nb::steal(PyObject_CallFunction(reinterpret_cast<PyObject*>(&PyRange_Type), "LLL", static_cast<long long>(range->start), static_cast<long long>(range->stop), static_cast<long long>(range->step)));
    }
    const void* identity = list.GetIdentity();
    if (auto found = memo.find(identity); found != memo.end())
    {
        return found->second;
    }
    nb::list items;
    memo.emplace(identity, items);
    list.ForEach([&](InternalValue&& item) {
        items.append(ToPythonImpl(item, memo));
        return true;
    });
    if (list.IsTuple())
    {
        auto tuple = nb::steal(PySequence_Tuple(items.ptr()));
        memo[identity] = tuple;
        return tuple;
    }
    return items;
}

nb::object MapToPython(const InternalValue& value, Memo& memo)
{
    const auto map = *jinja2::AsMap(value);
    if (const auto* host = map.GetHostObject())
    {
        return nb::borrow(static_cast<const PyHost*>(host)->Object());
    }
    // loop, a cycler, a namespace, a module: objects that are no dict in Python either
    if (map.GetObjectClass() || map.IsNamespace())
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
    for (const auto& entry : map.GetEntries())
    {
        auto key = ToPythonImpl(entry.key, memo);
        if (PyDict_SetItem(items.ptr(), key.ptr(), ToPythonImpl(entry.value, memo).ptr()) != 0)
        {
            throw nb::python_error();
        }
    }
    return items;
}

nb::object ToPythonImpl(const InternalValue& value, Memo& memo)
{
    switch (value.Kind())
    {
    case ValueKind::Undefined:
        return UndefinedToPython(value);
    case ValueKind::None:
        return nb::none();
    case ValueKind::Bool:
        return nb::bool_(*jinja2::GetIf<bool>(&value));
    case ValueKind::Int:
        return nb::steal(PyLong_FromLongLong(*jinja2::GetIf<int64_t>(&value)));
    case ValueKind::Double:
        return nb::steal(PyFloat_FromDouble(*jinja2::GetIf<double>(&value)));
    case ValueKind::String:
    {
        nb::object text;
        if (const auto narrow = jinja2::AsStringView<char>(value))
        {
            text = MakeStr(*narrow);
        }
        else if (const auto wide = jinja2::AsStringView<wchar_t>(value))
        {
            text = MakeStr(jinja2::ConvertString<std::string>(*wide));
        }
        else
        {
            text = MakeStr(jinja2::AsString(value));
        }
        if (value.IsMarkup())
        {
            return Names().markupType(text);
        }
        return text;
    }
    case ValueKind::BigInt:
    {
        const auto big = *jinja2::AsBigInt(value);
        // In hexadecimal, which sys.get_int_max_str_digits() does not limit
        auto hex = jinja2::MagnitudeInBase(big, 16);
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
    case ValueKind::List:
        return ListToPython(value, memo);
    case ValueKind::Map:
        return MapToPython(value, memo);
    case ValueKind::KeyValuePair:
    {
        const auto* pair = jinja2::GetIf<jinja2::KeyValuePair>(&value);
        return nb::make_tuple(ToPythonImpl(pair->key, memo), ToPythonImpl(pair->value, memo));
    }
    case ValueKind::HostValue:
    case ValueKind::Callable:
    case ValueKind::Renderer:
        return WrapTemplateValue(value);
    }
    return nb::none();
}

// The arguments of a call from Python, as the engine passes them
jinja2::CallParams ToCallParams(nb::args args, nb::kwargs kwargs)
{
    jinja2::CallParams params;
    params.posParams.reserve(args.size());
    for (auto arg : args)
    {
        params.posParams.push_back(ToEngine(arg));
    }
    for (auto [name, arg] : kwargs)
    {
        params.kwParams[Utf8(name)] = ToEngine(arg);
    }
    return params;
}
} // namespace

RenderState* CurrentRender() noexcept
{
    return t_render;
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

LeaseScope::LeaseScope(jinja2::RenderContext& context) noexcept
    : m_context(context)
    , m_outer(t_lease)
{
    t_lease = this;
}

LeaseScope::~LeaseScope()
{
    t_lease = m_outer;
    if (m_lease)
    {
        m_lease->active = false;
        // Each value lets go of what it refers to in the render, which may end now
        auto values = std::move(m_lease->values);
        for (auto* value : values)
        {
            value->Expire();
        }
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
        scope->m_lease = std::make_shared<CallLease>();
        scope->m_lease->context = &scope->m_context;
        scope->m_lease->collector = jinja2::CycleCollector::Current();
    }
    return scope->m_lease;
}

TemplateValue::TemplateValue(InternalValue value, std::shared_ptr<CallLease> lease)
    : m_value(std::move(value))
    , m_lease(std::move(lease))
{
    if (m_lease && m_lease->active)
    {
        m_lease->values.push_back(this);
    }
    else
    {
        // Made outside a call (a comparison): nothing keeps its render alive for it
        Expire();
    }
}

TemplateValue::~TemplateValue()
{
    if (m_lease && m_lease->active)
    {
        auto& values = m_lease->values;
        for (auto& value : values)
        {
            if (value == this)
            {
                value = values.back();
                values.pop_back();
                break;
            }
        }
    }
}

void TemplateValue::Expire() noexcept
{
    m_alive = false;
    m_value = InternalValue();
}

const InternalValue& TemplateValue::Value() const
{
    if (!m_alive)
    {
        RaiseTemplateRuntimeError("a value of the template was used after the call it was passed to ended");
    }
    return m_value;
}

nb::object TemplateValue::Call(nb::args args, nb::kwargs kwargs) const
{
    const auto& value = Value();
    const auto* callable = jinja2::GetIf<Callable>(&value);
    if (!callable)
    {
        if (const auto* host = jinja2::GetHostObject(value); host && host->IsCallable())
        {
            return ToPython(jinja2::host::MakeCall(value, *host).GetExpressionCallable()(ToCallParams(args, kwargs), *m_lease->context));
        }
        RaiseTypeError("'" + Repr() + "' object is not callable");
    }
    auto params = ToCallParams(args, kwargs);
    auto& context = *m_lease->context;
    const jinja2::CycleCollector::Reentry collector(m_lease->collector);
    // Python may catch what the callable throws and go on: the scopes a failed body left go
    const jinja2::HomeCall::Restore restore(context);
    InternalValue result;
    try
    {
        if (callable->GetType() == Callable::Type::Expression)
        {
            result = callable->GetExpressionCallable()(params, context);
        }
        else
        {
            result = InternalValue(jinja2::RenderToString(context.GetRendererCallback(), [&](jinja2::OutStream& stream) { callable->GetStatementCallable()(params, stream, context); }));
            result.SetMarkup(context.IsAutoescape());
        }
    }
    catch (const nb::python_error&)
    {
        throw;
    }
    catch (...)
    {
        RaiseCurrentEngineError();
    }
    return ToPython(result);
}

nb::object TemplateValue::GetAttr(const std::string& name) const
{
    const auto& value = Value();
    InternalValue result;
    try
    {
        result = jinja2::Subscript(value, name, m_lease->context);
    }
    catch (const nb::python_error&)
    {
        throw;
    }
    catch (...)
    {
        RaiseCurrentEngineError();
    }
    if (result.IsUndefined())
    {
        PyErr_Format(PyExc_AttributeError, "'%s' object has no attribute '%s'", TypeName().c_str(), name.c_str());
        throw nb::python_error();
    }
    return ToPython(result);
}

std::string TemplateValue::TypeName() const
{
    if (!m_alive)
    {
        return "TemplateValue";
    }
    return jinja2::Apply<jinja2::visitors::PythonTypeNameGetter>(m_value);
}

std::string TemplateValue::Str() const
{
    std::string result;
    jinja2::Apply<jinja2::visitors::ValueRenderer<char>>(Value(), result);
    return result;
}

std::string TemplateValue::Repr() const
{
    if (!m_alive)
    {
        return "<expired template value>";
    }
    std::string result;
    jinja2::Apply<jinja2::visitors::ValueRenderer<char>>(m_value, result, true);
    return result;
}

InternalValue ToEngine(nb::handle handle)
{
    PyObject* object = handle.ptr();
    if (object == Py_None)
    {
        return InternalValue(jinja2::NoneValue{});
    }
    if (object == Py_True || object == Py_False)
    {
        return InternalValue(object == Py_True);
    }
    if (PyUnicode_CheckExact(object))
    {
        return StrToEngine(object, false);
    }
    if (PyLong_CheckExact(object))
    {
        return IntToEngine(object);
    }
    if (PyFloat_CheckExact(object))
    {
        return InternalValue(PyFloat_AsDouble(object));
    }
    if (PyUnicode_Check(object))
    {
        // Markup is a str subclass: checked before the other subclasses, which are text
        const int markup = PyObject_IsInstance(object, Names().markupType.ptr());
        if (markup < 0)
        {
            throw nb::python_error();
        }
        return StrToEngine(object, markup != 0);
    }
    if (PyLong_Check(object))
    {
        return IntToEngine(object);
    }
    if (PyFloat_Check(object))
    {
        const double value = PyFloat_AsDouble(object);
        if (value == -1.0 && PyErr_Occurred())
        {
            throw nb::python_error();
        }
        return InternalValue(value);
    }
    // The common containers first: no question of their type
    if (PyDict_CheckExact(object) || PyList_CheckExact(object) || PyTuple_CheckExact(object))
    {
        return jinja2::MakeHostValue<PyHost>(handle, false);
    }
    if (nb::isinstance<TemplateValue>(handle))
    {
        return nb::cast<const TemplateValue&>(handle).Value();
    }
    if (IsUndefinedType(Py_TYPE(object)))
    {
        return UndefinedToEngine(object);
    }
    return jinja2::MakeHostValue<PyHost>(handle, false);
}

nb::object ToPython(const InternalValue& value)
{
    Memo memo;
    return ToPythonImpl(value, memo);
}

InternalValue BorrowedHostValue(nb::handle object)
{
    return jinja2::MakeHostValue<PyHost>(object, true);
}

InternalValue BorrowedCallable(nb::handle object)
{
    PyObject* callable = object.ptr();
    return InternalValue(Callable(Callable::UserCallable, [callable](const jinja2::CallParams& params, jinja2::RenderContext& context) { return CallPython(callable, params, context); }));
}

} // namespace jinja2cpp_py

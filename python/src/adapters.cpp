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
    // A StopIteration out of a host object's attribute or item in a call the template makes gives undefined
    // (docs/tasks/0479)
    if (PyErr_ExceptionMatches(PyExc_StopIteration))
    {
        throw PyStopIteration(nb::python_error());
    }
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

bool PyHost::IsDict() const
{
    return m_kind == Kind::Dict || PyDict_Check(m_object);
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

std::optional<std::string> PyHost::HtmlFormat(std::string_view spec) const
{
    if (m_kind != Kind::Other)
    {
        return std::nullopt;
    }
    if (PyObject_HasAttrString(m_object, "__html_format__") == 0 && !IsMarkupLike())
    {
        return std::nullopt;
    }
    // markupsafe's own formatter, so that its order (__html_format__, then __html__ without a spec) and its errors are
    // the ones Markup.format has
    static PyObject* const formatter = [] {
        const auto markupsafe = nb::module_::import_("markupsafe");
        return markupsafe.attr("EscapeFormatter")(markupsafe.attr("escape")).release().ptr();
    }();
    const auto text = Steal(PyUnicode_FromStringAndSize(spec.data(), static_cast<Py_ssize_t>(spec.size())));
    return Utf8(Steal(PyObject_CallMethod(formatter, "format_field", "OO", m_object, text.ptr())));
}

bool PyHost::IsNumber() const
{
    if (m_kind != Kind::Other)
    {
        return false;
    }
    const int number = PyObject_IsInstance(m_object, Names().numberType.ptr());
    if (number < 0)
    {
        ThrowPending();
    }
    return number != 0;
}

// int(x) and float(x) as Jinja2's do_int and do_float call them: a TypeError or ValueError is no value, any other
// exception is the template's. ToInt is the whole of do_int: int(x), then int(float(x)), where an OverflowError is
// also no value; an empty value tells the engine there is none, so it does not ask ToFloat again
std::optional<jinja2::Value> PyHost::ToInt() const
{
    if (m_kind != Kind::Other)
    {
        return std::nullopt;
    }
    const auto clearIf = [](bool overflowToo) {
        if (PyErr_ExceptionMatches(PyExc_TypeError) || PyErr_ExceptionMatches(PyExc_ValueError) || (overflowToo && PyErr_ExceptionMatches(PyExc_OverflowError)))
        {
            PyErr_Clear();
            return;
        }
        ThrowPending();
    };
    if (PyObject* const number = PyNumber_Long(m_object))
    {
        return ToEngine(nb::steal(number));
    }
    clearIf(false);
    PyObject* const asFloat = PyNumber_Float(m_object);
    if (!asFloat)
    {
        clearIf(true);
        return jinja2::Value();
    }
    PyObject* const number = PyNumber_Long(asFloat);
    Py_DECREF(asFloat);
    if (!number)
    {
        clearIf(true);
        return jinja2::Value();
    }
    return ToEngine(nb::steal(number));
}

std::optional<jinja2::Value> PyHost::ToFloat() const
{
    if (m_kind != Kind::Other)
    {
        return std::nullopt;
    }
    PyObject* const number = PyNumber_Float(m_object);
    if (!number)
    {
        if (PyErr_ExceptionMatches(PyExc_TypeError) || PyErr_ExceptionMatches(PyExc_ValueError))
        {
            PyErr_Clear();
            return std::nullopt;
        }
        ThrowPending();
    }
    return ToEngine(nb::steal(number));
}

namespace
{
// A sequence repeated by a count the way `bytes * n`, `bytearray * n`, `array * n` and `deque * n` repeat (Py_sq_repeat) is
// built by Python before the engine sees it: the length it would have counts against the sequence cap first

// len(x) of an object whose type has a length, nullopt for one without; an error __len__ raises propagates, so a
// guard never lets an operation through unchecked
std::optional<unsigned long long> LengthOf(PyObject* object)
{
    PyTypeObject* const type = Py_TYPE(object);
    if (!PyType_GetSlot(type, Py_sq_length) && !PyType_GetSlot(type, Py_mp_length))
    {
        return std::nullopt;
    }
    const Py_ssize_t length = PyObject_Size(object);
    if (length < 0)
    {
        ThrowPending();
    }
    return static_cast<unsigned long long>(length);
}

// The length a guard counts: the larger of len(x) and the length that the type providing the operation reads. A
// subclass of bytes, bytearray, array or deque that overrides __len__ still repeats and concatenates its storage (the
// inherited slot reads it), so the length slot of the furthest base that shares the operation's slot counts as well
std::optional<unsigned long long> GuardLength(PyObject* object, int operationSlot)
{
    auto length = LengthOf(object);
    PyTypeObject* const type = Py_TYPE(object);
    void* const operation = PyType_GetSlot(type, operationSlot);
    PyTypeObject* provider = type;
    while (auto* base = static_cast<PyTypeObject*>(PyType_GetSlot(provider, Py_tp_base)))
    {
        if (PyType_GetSlot(base, operationSlot) != operation)
        {
            break;
        }
        provider = base;
    }
    if (provider == type)
    {
        return length;
    }
    auto* storageLength = reinterpret_cast<lenfunc>(PyType_GetSlot(provider, Py_sq_length));
    if (!storageLength)
    {
        storageLength = reinterpret_cast<lenfunc>(PyType_GetSlot(provider, Py_mp_length));
    }
    if (!storageLength)
    {
        return length;
    }
    const Py_ssize_t storage = storageLength(object);
    if (storage < 0)
    {
        ThrowPending();
    }
    const auto counted = static_cast<unsigned long long>(storage);
    return length && *length > counted ? *length : counted;
}

// The bytes a buffer holds (bytes, bytearray, memoryview, array, mmap): what a bytes or bytearray concatenation or
// a `%s`/`%b` conversion copies, whatever its length in items. nullopt for an object without a buffer
std::optional<unsigned long long> BufferBytes(PyObject* object)
{
    if (!PyObject_CheckBuffer(object))
    {
        return std::nullopt;
    }
    Py_buffer view{};
    if (PyObject_GetBuffer(object, &view, PyBUF_RECORDS_RO) < 0)
    {
        PyErr_Clear();
        return std::nullopt;
    }
    const auto bytes = static_cast<unsigned long long>(view.len < 0 ? 0 : view.len);
    PyBuffer_Release(&view);
    return bytes;
}

bool IsByteString(PyObject* object)
{
    return PyBytes_Check(object) || PyByteArray_Check(object);
}

// A deque with a maxlen keeps at most that many items of a repeat or a concatenation: the bound of its result
unsigned long long BoundByMaxlen(PyObject* sequence, unsigned long long length)
{
    const int isDeque = PyObject_IsInstance(sequence, Names().dequeType.ptr());
    if (isDeque < 0)
    {
        ThrowPending();
    }
    if (isDeque == 0)
    {
        return length;
    }
    // deque's own descriptor: a subclass cannot make the bound larger than the storage keeps
    const auto descriptor = Steal(PyObject_GetAttrString(Names().dequeType.ptr(), "maxlen"));
    const auto maxlen = Steal(PyObject_CallMethod(descriptor.ptr(), "__get__", "O", sequence));
    if (maxlen.ptr() == Py_None)
    {
        return length;
    }
    const Py_ssize_t most = PyLong_AsSsize_t(maxlen.ptr());
    if (most == -1 && PyErr_Occurred())
    {
        ThrowPending();
    }
    const auto kept = static_cast<unsigned long long>(most < 0 ? 0 : most);
    return length < kept ? length : kept;
}

// sequence * count of a type that repeats (bytes, bytearray, array, deque): the result's length against the cap,
// before Python allocates it
void CheckRepeatSize(PyObject* sequence, PyObject* count)
{
    if (!PyType_GetSlot(Py_TYPE(sequence), Py_sq_repeat) || !PyIndex_Check(count))
    {
        return;
    }
    const auto length = GuardLength(sequence, Py_sq_repeat);
    if (!length || *length == 0)
    {
        return;
    }
    // Clamps an int too large for Py_ssize_t instead of raising
    const Py_ssize_t times = PyNumber_AsSsize_t(count, nullptr);
    if (times == -1 && PyErr_Occurred())
    {
        // A count that is no index (numpy's ndarray has __index__ but refuses it) makes this no repeat: Python decides.
        // Any other error (an __index__ that raises) propagates
        if (!PyErr_ExceptionMatches(PyExc_TypeError))
        {
            ThrowPending();
        }
        PyErr_Clear();
        return;
    }
    if (times <= 0)
    {
        return;
    }
    const auto repeats = static_cast<unsigned long long>(times);
    constexpr unsigned long long most = ~0ULL;
    CheckSequenceSize(BoundByMaxlen(sequence, *length > most / repeats ? most : *length * repeats));
}

// left + right of a type that concatenates (bytes, bytearray, array, deque): the sum of the lengths against the cap,
// so a template loop that doubles a value stops at the cap as a str or list does. Python concatenates by the left
// operand's slot only (`n * b` repeats by either), and a left deque's maxlen bounds the result
void CheckConcatSize(PyObject* left, PyObject* right)
{
    if (!PyType_GetSlot(Py_TYPE(left), Py_sq_concat))
    {
        return;
    }
    const auto leftLength = GuardLength(left, Py_sq_concat);
    auto rightLength = GuardLength(right, Py_sq_concat);
    // bytes + memoryview copies the view's bytes, not its items
    if (IsByteString(left))
    {
        if (const auto bytes = BufferBytes(right); bytes && (!rightLength || *bytes > *rightLength))
        {
            rightLength = bytes;
        }
    }
    if (leftLength && rightLength)
    {
        CheckSequenceSize(BoundByMaxlen(left, *leftLength + *rightLength));
    }
}

// The padding a printf-style bytes format adds: a width, written in the format or taken by `*` from the arguments,
// makes a conversion that long, and a precision does for a number (`%.*d` pads zeros; a string conversion truncates).
// `b'%*s' % (10**9, b'x')` would build it before the engine sees it, so the sum counts against the cap first, with
// the format's own text and the bytes a `%s`, `%b`, `%r` or `%a` copies from a buffer (a loop of `fmt % (v, v)` doubles
// v). Another value's own conversion (`__bytes__`, a repr) is its own cost. A str format is the engine's own
// (PercentFormat); a format Python rejects is left to Python's own error
void CheckFormatSize(PyObject* format, PyObject* args)
{
    const char* text = nullptr;
    Py_ssize_t size = 0;
    if (PyBytes_Check(format))
    {
        char* bytes = nullptr;
        if (PyBytes_AsStringAndSize(format, &bytes, &size) < 0)
        {
            ThrowPending();
        }
        text = bytes;
    }
    else if (PyByteArray_Check(format))
    {
        text = PyByteArray_AsString(format);
        size = PyByteArray_Size(format);
    }
    else
    {
        return;
    }
    const bool tuple = PyTuple_Check(args) != 0;
    const Py_ssize_t argCount = tuple ? PyTuple_Size(args) : 1;
    Py_ssize_t argIndex = 0;
    constexpr unsigned long long most = ~0ULL >> 2;
    unsigned long long total = static_cast<unsigned long long>(size);
    const auto add = [&](unsigned long long count) { total = total > most - count ? most : total + count; };
    // A star takes the next argument: an int, or Python raises
    const auto star = [&](bool width) -> unsigned long long {
        if (argIndex >= argCount)
        {
            return 0;
        }
        PyObject* const item = tuple ? PyTuple_GetItem(args, argIndex) : args;
        ++argIndex;
        if (!item || !PyLong_Check(item))
        {
            PyErr_Clear();
            return 0;
        }
        int overflow = 0;
        const long long value = PyLong_AsLongLongAndOverflow(item, &overflow);
        if (overflow != 0)
        {
            return overflow < 0 && !width ? 0 : most;
        }
        if (value == -1 && PyErr_Occurred())
        {
            ThrowPending();
        }
        if (value < 0)
        {
            // A negative width left-justifies to its magnitude
            return width ? static_cast<unsigned long long>(-(value + 1)) + 1 : 0;
        }
        return static_cast<unsigned long long>(value);
    };
    const auto digits = [&](Py_ssize_t& pos) {
        unsigned long long value = 0;
        while (pos < size && text[pos] >= '0' && text[pos] <= '9')
        {
            value = value > most / 10 ? most : (value * 10) + static_cast<unsigned long long>(text[pos] - '0');
            ++pos;
        }
        return value;
    };
    for (Py_ssize_t pos = 0; pos < size; ++pos)
    {
        if (text[pos] != '%')
        {
            continue;
        }
        ++pos;
        if (pos < size && text[pos] == '%')
        {
            continue;
        }
        // A mapping key, which may hold balanced parentheses: such a conversion takes args[key], not the next argument
        nb::object keyed;
        if (pos < size && text[pos] == '(')
        {
            int depth = 1;
            const Py_ssize_t keyStart = pos + 1;
            for (++pos; pos < size && depth > 0; ++pos)
            {
                depth += text[pos] == '(' ? 1 : (text[pos] == ')' ? -1 : 0);
            }
            if (depth == 0 && !tuple && PyMapping_Check(args))
            {
                const auto key = Steal(PyBytes_FromStringAndSize(text + keyStart, pos - 1 - keyStart));
                PyObject* const value = PyObject_GetItem(args, key.ptr());
                if (value)
                {
                    keyed = nb::steal(value);
                }
                else
                {
                    // A missing key is Python's KeyError, raised by the formatter itself
                    PyErr_Clear();
                }
            }
        }
        while (pos < size && (text[pos] == '-' || text[pos] == '+' || text[pos] == ' ' || text[pos] == '#' || text[pos] == '0'))
        {
            ++pos;
        }
        unsigned long long padding = 0;
        if (pos < size && text[pos] == '*')
        {
            padding = star(true);
            ++pos;
        }
        else
        {
            padding = digits(pos);
        }
        unsigned long long precision = 0;
        const bool hasPrecision = pos < size && text[pos] == '.';
        if (hasPrecision)
        {
            ++pos;
            if (pos < size && text[pos] == '*')
            {
                // A negative precision is ignored
                precision = star(false);
                ++pos;
            }
            else
            {
                precision = digits(pos);
            }
        }
        while (pos < size && (text[pos] == 'h' || text[pos] == 'l' || text[pos] == 'L'))
        {
            ++pos;
        }
        // A string conversion truncates to its precision; a number is padded to the larger of the two
        const char conversion = pos < size ? text[pos] : '\0';
        const bool copies = conversion == 's' || conversion == 'b';
        const bool quotes = conversion == 'r' || conversion == 'a';
        const bool truncates = copies || quotes || conversion == 'c';
        if (!truncates && precision > padding)
        {
            padding = precision;
        }
        // The converted value: args[key], or the next argument; `pos` now stands on the conversion
        PyObject* value = keyed.ptr();
        if (!value && conversion != '%')
        {
            value = argIndex < argCount ? (tuple ? PyTuple_GetItem(args, argIndex) : args) : nullptr;
            ++argIndex;
        }
        if (value && (copies || quotes))
        {
            if (auto bytes = BufferBytes(value))
            {
                // repr of bytes escapes each byte to at most four characters, plus b'' around it
                unsigned long long copied = quotes ? (*bytes > most / 4 ? most : (*bytes * 4) + 3) : *bytes;
                if (copies && hasPrecision && precision < copied)
                {
                    copied = precision;
                }
                add(copied);
            }
        }
        add(padding);
        if (total >= most)
        {
            break;
        }
    }
    CheckSequenceSize(total);
}
// A bytes, bytearray, array or deque an operation built: its length against the cap, so that growth the guards above
// cannot predict (a type's own conversion) stops at the next step, as a str or list does
void CheckResultSize(PyObject* result)
{
    if (!IsByteString(result))
    {
        const int sequence = PyObject_IsInstance(result, Names().arrayType.ptr());
        const int queue = sequence > 0 ? 0 : PyObject_IsInstance(result, Names().dequeType.ptr());
        if (sequence < 0 || queue < 0)
        {
            ThrowPending();
        }
        if (sequence == 0 && queue == 0)
        {
            return;
        }
    }
    if (const auto length = GuardLength(result, Py_sq_length))
    {
        CheckSequenceSize(*length);
    }
}
} // namespace

// The operators as Python runs them: PyNumber_* and PyObject_RichCompare try the reflected method, the subclass first
// rule and NotImplemented themselves, and a TypeError is Python's own. Only the exact dict, list and tuple decline: the
// engine reads those by its own rules
std::optional<jinja2::Value> PyHost::Operate(jinja2::HostOperator op, const jinja2::Value& other, jinja2::HostOperandSide side) const
{
    using jinja2::HostOperator;
    if (m_kind != Kind::Other)
    {
        return std::nullopt;
    }
    if (op == HostOperator::Negative)
    {
        return ToEngine(Steal(PyNumber_Negative(m_object)));
    }
    if (op == HostOperator::Positive)
    {
        return ToEngine(Steal(PyNumber_Positive(m_object)));
    }
    const auto pyOther = ToPython(other);
    PyObject* left = m_object;
    PyObject* right = pyOther.ptr();
    if (side == jinja2::HostOperandSide::Right)
    {
        std::swap(left, right);
    }
    PyObject* result = nullptr;
    switch (op)
    {
    case HostOperator::Add:
        CheckConcatSize(left, right);
        result = PyNumber_Add(left, right);
        break;
    case HostOperator::Subtract:
        result = PyNumber_Subtract(left, right);
        break;
    case HostOperator::Multiply:
        CheckRepeatSize(left, right);
        CheckRepeatSize(right, left);
        result = PyNumber_Multiply(left, right);
        break;
    case HostOperator::TrueDivide:
        result = PyNumber_TrueDivide(left, right);
        break;
    case HostOperator::FloorDivide:
        result = PyNumber_FloorDivide(left, right);
        break;
    case HostOperator::Remainder:
        CheckFormatSize(left, right);
        result = PyNumber_Remainder(left, right);
        break;
    case HostOperator::Power:
        result = PyNumber_Power(left, right, Py_None);
        break;
    case HostOperator::Less:
        result = PyObject_RichCompare(left, right, Py_LT);
        break;
    case HostOperator::LessEqual:
        result = PyObject_RichCompare(left, right, Py_LE);
        break;
    case HostOperator::Greater:
        result = PyObject_RichCompare(left, right, Py_GT);
        break;
    case HostOperator::GreaterEqual:
        result = PyObject_RichCompare(left, right, Py_GE);
        break;
    default:
        return std::nullopt;
    }
    auto owned = Steal(result);
    CheckResultSize(owned.ptr());
    return ToEngine(std::move(owned));
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
nb::object StubClass(const char* name)
{
    // A class of that name, for an object of the engine that has no Python class: loop, a macro, a cycler
    // `function` is a builtin type to object_type_repr ('function object'); the others take the module of the caller
    const bool builtin = std::string_view(name) == "function";
    return Steal(PyObject_CallFunction(reinterpret_cast<PyObject*>(&PyType_Type), builtin ? "s(O){s:s}" : "s(O){}", name, reinterpret_cast<PyObject*>(&PyBaseObject_Type), "__module__", "builtins"));
}

nb::object Stub(const char* name)
{
    return Steal(PyObject_CallNoArgs(StubClass(name).ptr()));
}

// A Python object of the kind of `obj`, for is_safe_attribute to look at: the value itself when it is
// small, an empty container otherwise (the checks look at the type; converting a list for each refused
// name would cost its length). A macro or `caller` reaches the policy as an empty value (sandbox_policy.h),
// which is None here: is_safe_attribute judges None's attributes for it
nb::object Representative(const jinja2::Value& obj, bool& synthetic)
{
    if (const auto* list = obj.GetIf<jinja2::GenericList>())
    {
        // A set and a dict view the template made are Python's own types for the policy (convert.cpp ViewToPython):
        // an empty one of the type, so that `add` of a set or `isdisjoint` of a view is asked about, not read unasked
        if (const char* viewClass = list->GetViewClass())
        {
            const std::string_view view(viewClass);
            if (view == "set")
            {
                return Steal(PySet_New(nullptr));
            }
            if (view == "dict_keys" || view == "dict_values" || view == "dict_items")
            {
                const auto source = Steal(PyDict_New());
                const std::string method(view.substr(5));
                return Steal(PyObject_CallMethod(source.ptr(), method.c_str(), nullptr));
            }
        }
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
            // A range and a groupby group have attributes of their own, which the policy judges by name: a real range, and
            // an object of Jinja2's _GroupTuple class (whose value is None: the engine builds none for the question)
            if (std::string_view(objectClass) == "range")
            {
                return Steal(PyObject_CallFunction(reinterpret_cast<PyObject*>(&PyRange_Type), "i", 0));
            }
            if (std::string_view(objectClass) == "_GroupTuple")
            {
                const auto members = Steal(Py_BuildValue("{s:O,s:O}", "grouper", Py_None, "list", Py_None));
                const auto type = Steal(PyObject_CallFunction(reinterpret_cast<PyObject*>(&PyType_Type), "s(O)O", "_GroupTuple", reinterpret_cast<PyObject*>(&PyTuple_Type), members.ptr()));
                return Steal(PyObject_CallNoArgs(type.ptr()));
            }
            synthetic = true;
            if (std::string_view(objectClass) == "type")
            {
                // A class object (`dict`, `cycler`, `x.__class__`): a type for is_safe_attribute, which refuses `mro` of one
                return StubClass(objectClass);
            }
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

// Whether the policy gets `obj` as the stand-in of a callable that is no class: a function (the sandbox's range, a
// global function, a host callable) or a macro
bool IsCallableStandIn(const jinja2::Value& obj)
{
    if (obj.GetIf<jinja2::UserCallable>())
    {
        return true;
    }
    const auto* map = obj.GetIf<jinja2::GenericMap>();
    const char* objectClass = map ? map->GetObjectClass() : nullptr;
    return objectClass && (std::string_view(objectClass) == "function" || std::string_view(objectClass) == "Macro");
}

// The ISandboxPolicy of a SandboxedEnvironment: the environment's own is_safe_attribute and wrap_str_format, asked
// for the engine's values. It borrows the environment, which holds the engine environment that holds this
class PySandboxPolicy final : public jinja2::ISandboxPolicy
{
public:
    explicit PySandboxPolicy(PyObject* environment)
        : m_environment(environment)
    {
    }

    [[nodiscard]] std::optional<jinja2::Value> NativeAttribute(const jinja2::Value& obj, std::string_view name) const override
    {
        // Python's objects have attributes the engine's values have not: __class__, __init__, ...
        // A public name is the environment's to judge only when its is_safe_attribute is not Jinja2's, which
        // refuses private names and the methods of lists and dicts that change them (the engine does the latter)
        // `mro` is asked too: Jinja2 refuses it of a class, which only the stand-in of a class object shows
        const bool isPrivate = !name.empty() && name[0] == '_';
        if (!isPrivate && name != "mro" && !JudgesPublicNames())
        {
            return std::nullopt;
        }
        bool synthetic = false;
        const auto representative = Representative(obj, synthetic);
        const auto pyName = MakeStr(name);
        if (name == "mro" && IsCallableStandIn(obj))
        {
            // The engine's class objects have mro; a callable Python holds as no class (the sandbox's range is
            // safe_range, a function; a macro) has not, and Jinja2's getattr gives its undefined. Other objects
            // (namespace(), self, a module) may hold an item or macro named mro: the engine reads it
            const auto undefined = Steal(PyObject_GetAttrString(m_environment, "undefined"));
            const auto args = Steal(PyTuple_New(0));
            const auto kwargs = Steal(Py_BuildValue("{s:O,s:O}", "obj", representative.ptr(), "name", pyName.ptr()));
            return ToEngine(Steal(PyObject_Call(undefined.ptr(), args.ptr(), kwargs.ptr())));
        }
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
        // None: the engine reads the attribute itself; else (replacement,), what unsafe_undefined gave, an Undefined or not
        const auto checked = Steal(PyObject_CallMethod(m_environment, "_sandbox_native_attribute", "OOO", representative.ptr(), pyName.ptr(), owned.ptr()));
        if (checked.is_none())
        {
            return std::nullopt;
        }
        return ToEngine(nb::borrow(PyTuple_GetItem(checked.ptr(), 0)));
    }

    [[nodiscard]] std::optional<jinja2::Value> Format(const jinja2::Value& self, bool formatMap, const jinja2::UserCallableParams& params) const override
    {
        // str.format and str.format_map of the engine's strings: Python's, through the environment's wrap_str_format
        const auto text = ToPython(self);
        const auto method = Steal(PyObject_GetAttrString(text.ptr(), formatMap ? "format_map" : "format"));
        auto wrapped = Steal(PyObject_CallMethod(m_environment, "wrap_str_format", "O", method.ptr()));
        if (wrapped.is_none())
        {
            wrapped = method;
        }
        return CallPython(wrapped.ptr(), params);
    }

private:
    // What the entry into the render found, when this environment is the render's sandbox; else (an engine value of
    // another environment's render, a call outside a render) Python's answer now: refusing more is the safe error
    [[nodiscard]] bool JudgesPublicNames() const
    {
        if (const auto* render = CurrentRender(); render && render->sandbox == m_environment)
        {
            return render->sandboxAllNames;
        }
        return SandboxJudgesPublicNames(nb::handle(m_environment));
    }

    PyObject* m_environment;
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

std::shared_ptr<const jinja2::ISandboxPolicy> MakePythonSandboxPolicy(nb::handle environment)
{
    return std::make_shared<PySandboxPolicy>(environment.ptr());
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
        if (error.matches(PyExc_StopIteration))
        {
            // Marked for the engine: a call the template makes it escapes gives undefined (docs/tasks/0479)
            throw PyStopIteration(std::move(error));
        }
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

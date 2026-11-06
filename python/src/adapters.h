#ifndef JINJA2CPP_PYTHON_ADAPTERS_H
#define JINJA2CPP_PYTHON_ADAPTERS_H

#include "common.h"

#include "host_object.h"

#include "sandbox_policy.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace jinja2cpp_py
{
using jinja2::CallParams;
using jinja2::InternalValue;
using jinja2::ListAdapter;
using jinja2::RenderContext;

// A Python object read by the engine as Python reads it (0248 D4): one adapter for every kind
// of object, which asks Python for each operation, so a dict, a list, a tuple, a namedtuple, a
// generator, a callable or any other object behaves as itself, and mutation is seen both ways.
// The common kinds take shortcuts with the same results
class PyHost final : public jinja2::IHostObject
{
public:
    enum class Kind : unsigned char
    {
        Dict,
        List,
        Tuple,
        Other
    };

    // A strong reference, or a borrowed one the caller keeps alive (the environment's filters)
    PyHost(nb::handle object, bool borrowed);
    ~PyHost() override;
    PyHost(const PyHost&) = delete;
    PyHost& operator=(const PyHost&) = delete;
    PyHost(PyHost&&) = delete;
    PyHost& operator=(PyHost&&) = delete;

    [[nodiscard]] PyObject* Object() const noexcept { return m_object; }

    [[nodiscard]] const char* TypeName() const override;
    [[nodiscard]] std::string Str() const override;
    [[nodiscard]] std::string Repr() const override;
    [[nodiscard]] std::optional<InternalValue> GetAttr(const std::string& name) const override;
    [[nodiscard]] std::optional<InternalValue> GetItem(const InternalValue& key) const override;
    [[nodiscard]] std::optional<InternalValue> GetSlice(const InternalValue& start, const InternalValue& stop, const InternalValue& step) const override;
    [[nodiscard]] bool IsCallable() const override;
    InternalValue Call(const CallParams& params, RenderContext& context) const override;
    [[nodiscard]] bool IsIterable() const override;
    [[nodiscard]] std::optional<ListAdapter> Iterate() const override;
    [[nodiscard]] std::optional<ListAdapter> Reversed() const override;
    [[nodiscard]] std::optional<bool> Contains(const InternalValue& item) const override;
    [[nodiscard]] std::optional<size_t> Len() const override;
    [[nodiscard]] bool Truth() const override;
    [[nodiscard]] std::optional<bool> Equals(const InternalValue& other) const override;
    [[nodiscard]] std::optional<size_t> Hash() const override;
    [[nodiscard]] const void* Identity() const override { return m_object; }
    [[nodiscard]] bool IsMapping() const override;
    [[nodiscard]] bool IsSequence() const override;
    [[nodiscard]] bool IsTuple() const override;
    [[nodiscard]] bool IsMarkupLike() const override;
    [[nodiscard]] std::string Html() const override;

private:
    PyObject* m_object;
    bool m_borrowed;
    Kind m_kind;
    // -1 unknown, else whether the object has __html__, and is a collections.abc.Mapping
    mutable signed char m_markupLike = -1;
    mutable signed char m_mapping = -1;
    mutable std::string m_typeName;
};

// The sandbox policy the engine asks for the values it makes (src/sandbox_policy.h) in the renders of the
// SandboxedEnvironment `environment`, which it borrows
// Whether the sandboxed environment judges the public names of the engine's values too (its is_safe_attribute is not
// Jinja2's): read again at each load, as the method may be replaced after the first
bool SandboxJudgesPublicNames(nb::handle environment);
std::shared_ptr<const jinja2::ISandboxPolicy> MakePythonSandboxPolicy(nb::handle environment, bool allNames);

// The PyHost a value of the engine carries, if any
const PyHost* AsPyHost(const InternalValue& value);

// How many Python callables the engine has called (filters, tests, globals, finalize, the
// autoescape callable, the gettext callables): the fallback retries a render only while this
// has not moved (docs/python.md, "Fallback"). A loader's get_source is not counted
std::uint64_t PythonCallCount() noexcept;
void NotePythonCall() noexcept;

// Calls `callable` with the engine's arguments, as a template calls it
InternalValue CallPython(PyObject* callable, const CallParams& params, RenderContext& context);

} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ADAPTERS_H

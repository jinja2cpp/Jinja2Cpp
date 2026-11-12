#ifndef JINJA2CPP_PYTHON_ADAPTERS_H
#define JINJA2CPP_PYTHON_ADAPTERS_H

#include "common.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/sandbox_policy.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace jinja2cpp_py
{
// A Python object read by the engine as Python reads it (0248 D4), through the public host-object
// interface (jinja2cpp/host_object.h, docs/tasks/0357): one adapter for every kind of object, which
// asks Python for each operation, so a dict, a list, a tuple, a namedtuple, a generator, a callable
// or any other object behaves as itself, and mutation is seen both ways. The common kinds take
// shortcuts with the same results, and write their scalars straight into the engine (HostResult)
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

    // A strong reference, or a borrowed one the caller keeps alive
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
    [[nodiscard]] std::optional<jinja2::Value> GetAttr(std::string_view name) const override;
    [[nodiscard]] std::optional<jinja2::Value> GetItem(const jinja2::Value& key) const override;
    [[nodiscard]] bool GetAttrTo(std::string_view name, jinja2::HostResult& out) const override;
    [[nodiscard]] bool GetItemTo(std::string_view key, jinja2::HostResult& out) const override;
    [[nodiscard]] bool ItemAtTo(size_t index, jinja2::HostResult& out) const override;
    [[nodiscard]] std::optional<jinja2::Value> GetSlice(const jinja2::Value& start, const jinja2::Value& stop, const jinja2::Value& step) const override;
    [[nodiscard]] bool IsCallable() const override;
    [[nodiscard]] jinja2::Value Call(const jinja2::UserCallableParams& params) const override;
    [[nodiscard]] bool IsIterable() const override;
    [[nodiscard]] std::optional<jinja2::GenericList> Iterate() const override;
    [[nodiscard]] std::optional<jinja2::GenericList> Reversed() const override;
    [[nodiscard]] std::optional<bool> Contains(const jinja2::Value& item) const override;
    [[nodiscard]] std::optional<size_t> Len() const override;
    [[nodiscard]] bool Truth() const override;
    [[nodiscard]] std::optional<bool> Equals(const jinja2::Value& other) const override;
    [[nodiscard]] std::optional<size_t> Hash() const override;
    [[nodiscard]] const void* Identity() const override { return m_object; }
    [[nodiscard]] bool IsMapping() const override;
    [[nodiscard]] bool IsSequence() const override;
    [[nodiscard]] bool IteratesByIndex() const override;
    [[nodiscard]] bool IsTuple() const override;
    [[nodiscard]] bool IsMarkupLike() const override;
    [[nodiscard]] std::string Html() const override;
    [[nodiscard]] bool IsNumber() const override;
    [[nodiscard]] std::optional<jinja2::Value> ToInt() const override;
    [[nodiscard]] std::optional<jinja2::Value> ToFloat() const override;

private:
    // The attribute `name`, checked by the sandbox of the render when there is one; null when there is none
    [[nodiscard]] nb::object Attribute(std::string_view name) const;
    // The item of a str key; null when there is none
    [[nodiscard]] nb::object ItemOfName(std::string_view key) const;

    PyObject* m_object;
    bool m_borrowed;
    Kind m_kind;
    // -1 unknown, else whether the object has __html__, and is a collections.abc.Mapping
    mutable signed char m_markupLike = -1;
    mutable signed char m_mapping = -1;
    mutable std::string m_typeName;
};

// The PyHost a value of the engine carries, if any
const PyHost* AsPyHost(const jinja2::Value& value);

// Whether the sandboxed environment judges the public names of the engine's values too (its is_safe_attribute is not
// Jinja2's): read again at each load, as the method may be replaced after the first
bool SandboxJudgesPublicNames(nb::handle environment);
// The sandbox policy the engine asks for the values it makes (Settings::sandboxPolicy) in the renders of the
// SandboxedEnvironment `environment`, which it borrows
std::shared_ptr<const jinja2::ISandboxPolicy> MakePythonSandboxPolicy(nb::handle environment, bool allNames);

// Calls `callable` with the engine's arguments (ArgInfo::VarArgs and ArgInfo::VarKwArgs), as a template calls it
jinja2::Value CallPython(PyObject* callable, const jinja2::UserCallableParams& params);

} // namespace jinja2cpp_py

#endif // JINJA2CPP_PYTHON_ADAPTERS_H

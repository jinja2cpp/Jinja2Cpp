// Shared helpers of the embedding lifecycle scenario suite (embedding_lifecycle_*_test.cpp, task 0223)
#ifndef JINJA2CPP_TEST_EMBEDDING_LIFECYCLE_TEST_H
#define JINJA2CPP_TEST_EMBEDDING_LIFECYCLE_TEST_H

// Scenario suite for the embedding lifecycle contract (docs/api-2.0.md 3.10, task 0223): what a host may rely on when
// environments, templates, renders, modules and host callbacks overlap in time. Every row asserts an output, an error
// code and location, or a release (a weak_ptr that expired); a loop that only checks for "no crash" is not a row.
// The expected text of the rows with template-only semantics (E1, E6, F1-F5, D1) is Python Jinja2 3.1.6's.
// Rows that wait on other tasks are added by them: A5 and D5 (0317), E3 (0318), A6 and C7 (0264). F6 is not here: on
// master a render started by a callable does not see the global the outer render changed in place (0211 decision).
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace jinja2;

namespace embedding_lifecycle
{
// A one-shot gate: C++17 has no std::latch. Waiting gives up after a generous time, so a broken row fails and does not hang
class Latch
{
public:
    explicit Latch(int count = 1)
        : m_count(count)
    {
    }

    void CountDown()
    {
        {
            std::scoped_lock lock(m_mutex);
            if (m_count > 0)
            {
                --m_count;
            }
        }
        m_cv.notify_all();
    }

    [[nodiscard]] bool Wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, std::chrono::seconds(30), [this] { return m_count == 0; });
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_count;
};

// A handler with settable text and modification time per file, and thread-safe counters. `reportsTime` false behaves
// like MemoryFileSystem: no date, so a cached template is never reloaded
class CountingFs : public IFilesystemHandler
{
public:
    explicit CountingFs(bool reportsTime = false)
        : m_reportsTime(reportsTime)
    {
    }

    void Set(const std::string& name, std::string text)
    {
        std::scoped_lock lock(m_mutex);
        m_files[name].text = std::move(text);
    }
    //! Moves the modification time of the file on by one second
    void Touch(const std::string& name)
    {
        std::scoped_lock lock(m_mutex);
        ++m_files[name].seconds;
    }
    //! Called from OpenStream and OpenWStream with the file name, after it is counted and with no lock held
    void SetOpenHook(std::function<void(const std::string&)> hook)
    {
        std::scoped_lock lock(m_mutex);
        m_hook = std::move(hook);
    }
    int Opens(const std::string& name) const
    {
        std::scoped_lock lock(m_mutex);
        const auto p = m_opens.find(name);
        return p == m_opens.end() ? 0 : p->second;
    }
    int WideOpens(const std::string& name) const
    {
        std::scoped_lock lock(m_mutex);
        const auto p = m_wideOpens.find(name);
        return p == m_wideOpens.end() ? 0 : p->second;
    }
    int TotalOpens(const std::string& name) const { return Opens(name) + WideOpens(name); }
    int DateChecks() const
    {
        std::scoped_lock lock(m_mutex);
        return m_dateChecks;
    }

    CharFileStreamPtr OpenStream(const std::string& name) const override
    {
        std::optional<std::string> text = Open(name, m_opens);
        if (!text)
        {
            return CharFileStreamPtr(nullptr, [](std::istream*) {});
        }
        return CharFileStreamPtr(new std::istringstream(*text), [](std::istream* s) { delete s; });
    }

    WCharFileStreamPtr OpenWStream(const std::string& name) const override
    {
        std::optional<std::string> text = Open(name, m_wideOpens);
        if (!text)
        {
            return WCharFileStreamPtr(nullptr, [](std::wistream*) {});
        }
        return WCharFileStreamPtr(new std::wistringstream(std::wstring(text->begin(), text->end())), [](std::wistream* s) { delete s; });
    }

    std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const override
    {
        std::scoped_lock lock(m_mutex);
        ++m_dateChecks;
        const auto p = m_files.find(name);
        if (!m_reportsTime || p == m_files.end())
        {
            return std::nullopt;
        }
        return std::chrono::system_clock::time_point(std::chrono::seconds(1000000 + p->second.seconds));
    }

private:
    struct File
    {
        std::string text;
        int seconds = 0;
    };

    std::optional<std::string> Open(const std::string& name, std::map<std::string, int>& counter) const
    {
        std::function<void(const std::string&)> hook;
        std::optional<std::string> text;
        {
            std::scoped_lock lock(m_mutex);
            ++counter[name];
            hook = m_hook;
            const auto p = m_files.find(name);
            if (p != m_files.end())
            {
                text = p->second.text;
            }
        }
        if (hook)
        {
            hook(name);
        }
        return text;
    }

    const bool m_reportsTime;
    mutable std::mutex m_mutex;
    std::map<std::string, File> m_files;
    mutable std::map<std::string, int> m_opens;
    mutable std::map<std::string, int> m_wideOpens;
    mutable int m_dateChecks = 0;
    std::function<void(const std::string&)> m_hook;
};

// A watched object: the strong reference moves into whatever the row wants to observe (a callable in the globals, a
// handler), and the row asserts that the weak one expires
class Sentinel
{
public:
    Sentinel()
        : m_owner(std::make_shared<int>(0))
        , m_watch(m_owner)
    {
    }

    std::shared_ptr<int> Hold() { return std::move(m_owner); }
    [[nodiscard]] bool Expired() const { return m_watch.expired(); }

private:
    std::shared_ptr<int> m_owner;
    std::weak_ptr<int> m_watch;
};

// Failures found on worker threads: gtest assertions stay on the test's own thread
class Failures
{
public:
    void Add(std::string text)
    {
        std::scoped_lock lock(m_mutex);
        if (m_first.empty())
        {
            m_first = std::move(text);
        }
        ++m_count;
    }
    [[nodiscard]] std::string Summary() const
    {
        std::scoped_lock lock(m_mutex);
        return m_count == 0 ? std::string() : std::to_string(m_count) + " failures, first: " + m_first;
    }

private:
    mutable std::mutex m_mutex;
    std::string m_first;
    int m_count = 0;
};

// A callable that stores every value it receives as `v`
struct Kept
{
    std::mutex mutex;
    std::vector<Value> values;
};

inline UserCallable KeepCallable(const std::shared_ptr<Kept>& kept)
{
    return UserCallable(
        [kept](const UserCallableParams& p) {
            std::scoped_lock lock(kept->mutex);
            kept->values.push_back(p["v"]);
            return Value(std::string());
        },
        { ArgInfo{ "v" } });
}

template<typename CharT>
struct Tag
{
};

template<typename CharT>
std::basic_string<CharT> Str(const char* text)
{
    const std::string_view view(text);
    return std::basic_string<CharT>(view.begin(), view.end());
}

inline Result<Template> LoadFrom(TemplateEnv& env, const std::string& name, Tag<char>)
{
    return env.LoadTemplate(name);
}

inline ResultW<TemplateW> LoadFrom(TemplateEnv& env, const std::string& name, Tag<wchar_t>)
{
    return env.LoadTemplateW(name);
}

inline Result<Template> FromStringIn(TemplateEnv& env, const char* source, Tag<char>)
{
    return env.FromString(std::string_view(source));
}

inline ResultW<TemplateW> FromStringIn(TemplateEnv& env, const char* source, Tag<wchar_t>)
{
    return env.FromString(Str<wchar_t>(source));
}

template<typename R>
std::string Out(const R& result)
{
    return result ? std::string(*result) : "error: " + result.error().ToString();
}

// The text a render made, or its error's, in the template's character type. Used instead of
// value(): a value() in a lambda took GCC over 4 GB to compile under ASan
template<typename R>
auto Text(const R& result) -> std::decay_t<decltype(*result)>
{
    using CharT = typename std::decay_t<decltype(*result)>::value_type;
    return result ? *result : Str<CharT>("error: ") + result.error().ToString();
}

// Text of a value, the way the rows compare them; maps list `key:value,`
inline std::string Dump(const Value& v)
{
    if (const auto* sv = std::get_if<std::string_view>(&v.GetData()))
    {
        return std::string(*sv);
    }
    if (v.IsString())
    {
        return v.AsString();
    }
    if (v.IsInt())
    {
        return std::to_string(v.AsInt());
    }
    if (v.IsList())
    {
        std::string text = "[";
        for (const auto& item : v.ToGenericList())
        {
            text += Dump(item) + ",";
        }
        return text + "]";
    }
    if (v.IsMap())
    {
        const auto map = v.ToGenericMap();
        std::string text = "{";
        for (const auto& key : map.GetKeys())
        {
            text += key + ":" + Dump(map.GetValueByName(key)) + ",";
        }
        return text + "}";
    }
    return v.IsNone() ? "<none>" : "<other>";
}

inline Value CallValue(const Value& fn, ValuesList pos = {})
{
    UserCallableParams params;
    params.extraPosArgs = std::move(pos);
    return fn.As<UserCallable>().callable(params);
}

// A host record reflected by the E1 row
struct Person
{
    std::string name;
    std::vector<std::string> tags;
};
} // namespace embedding_lifecycle

namespace jinja2
{
template<>
struct TypeReflection<embedding_lifecycle::Person> : TypeReflected<embedding_lifecycle::Person>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "name", [](const embedding_lifecycle::Person& obj) { return Reflect(obj.name); } },
            { "tags", [](const embedding_lifecycle::Person& obj) { return Reflect(obj.tags); } },
        };
        return accessors;
    }
};
} // namespace jinja2

#endif // JINJA2CPP_TEST_EMBEDDING_LIFECYCLE_TEST_H

#include "loaders.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/utils/i_comparable.h>

#include <chrono>
#include <cstdint>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2cpp_py
{
namespace
{
using jinja2::CharFileStreamPtr;
using jinja2::WCharFileStreamPtr;
using TimePoint = std::chrono::system_clock::time_point;

CharFileStreamPtr TextStream(std::string text)
{
    return CharFileStreamPtr(new std::istringstream(std::move(text)), [](std::istream* stream) { delete stream; });
}

WCharFileStreamPtr NoWideStream()
{
    return WCharFileStreamPtr(nullptr, [](std::wistream* stream) { delete stream; });
}

// A version of each template's source the engine compares to decide on a reload
// (Settings::autoReload): it grows when the loader says the source changed, as Jinja2's
// uptodate() returning False
class Versions
{
public:
    TimePoint Current(const std::string& name, bool changed) const
    {
        auto& version = m_versions[name];
        if (changed || version == 0)
        {
            version = ++m_last;
        }
        return TimePoint(std::chrono::seconds(version));
    }

private:
    mutable std::unordered_map<std::string, std::int64_t> m_versions;
    mutable std::int64_t m_last = 0;
};

// Whether the pending Python error is a TemplateNotFound, ours or Jinja2's: then it is cleared, and
// the exception, with its traceback, goes to `kept` when given
bool PendingIsNotFound(PyRef* kept = nullptr)
{
    PyObject* type = nullptr;
    PyObject* value = nullptr;
    PyObject* traceback = nullptr;
    PyErr_Fetch(&type, &value, &traceback);
    bool notFound = false;
    if (type)
    {
        const auto mro = nb::steal(PyObject_GetAttrString(type, "__mro__"));
        if (mro)
        {
            for (auto base : mro)
            {
                notFound = notFound || IsJinjaClass(base, "TemplateNotFound");
            }
        }
        else
        {
            PyErr_Clear();
        }
    }
    if (notFound)
    {
        if (kept)
        {
            // Kept without its traceback and context: their frames (the caller's locals, the template)
            // would live as long as the loader, and make cycles the collector may not see
            PyErr_NormalizeException(&type, &value, &traceback);
            if (value)
            {
                PyException_SetTraceback(value, Py_None);
                PyException_SetContext(value, nullptr);
            }
            *kept = PyRef::Steal(std::exchange(value, nullptr));
        }
        Py_XDECREF(type);
        Py_XDECREF(value);
        Py_XDECREF(traceback);
        return true;
    }
    PyErr_Restore(type, value, traceback);
    return false;
}

// DictLoader: mapping[name]; a change of the source reloads the template, as its uptodate does
class DictHandler final : public jinja2::IFilesystemHandler
{
public:
    // The loader, whose `mapping` is read at each use: it may be replaced
    explicit DictHandler(PyObject* loader)
        : m_loader(loader)
    {
    }

    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string& name) const override
    {
        auto source = Read(name);
        if (!source)
        {
            return CharFileStreamPtr(nullptr, [](std::istream* stream) { delete stream; });
        }
        return TextStream(std::move(*source));
    }
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string& /*name*/) const override { return NoWideStream(); }
    [[nodiscard]] std::optional<TimePoint> GetLastModificationDate(const std::string& name) const override
    {
        auto source = Read(name);
        auto& seen = m_seen[name];
        const bool changed = source != seen;
        seen = std::move(source);
        return m_versions.Current(name, changed);
    }
    [[nodiscard]] bool IsEqual(const jinja2::IComparable& other) const override
    {
        const auto* handler = dynamic_cast<const DictHandler*>(&other);
        return handler && handler->m_loader == m_loader;
    }

    std::optional<std::string> Read(const std::string& name) const
    {
        const auto key = MakeStr(name);
        const auto mapping = nb::steal(PyObject_GetAttrString(m_loader, "mapping"));
        if (!mapping)
        {
            throw nb::python_error();
        }
        PyObject* source = PyObject_GetItem(mapping.ptr(), key.ptr());
        if (!source)
        {
            if (PyErr_ExceptionMatches(PyExc_KeyError))
            {
                PyErr_Clear();
                return std::nullopt;
            }
            throw nb::python_error();
        }
        return Utf8(nb::steal(source));
    }

private:
    PyObject* m_loader;
    mutable std::unordered_map<std::string, std::optional<std::string>> m_seen;
    Versions m_versions;
};

// A FileSystemLoader directory with an encoding other than UTF-8: the bytes of
// ConfinedFileSystem decoded by Python
class DecodingHandler final : public jinja2::IFilesystemHandler
{
public:
    DecodingHandler(std::string root, std::string encoding)
        : m_fs(std::move(root))
        , m_encoding(std::move(encoding))
    {
    }

    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string& name) const override
    {
        auto bytes = m_fs.OpenByteStream(name);
        if (!bytes)
        {
            return CharFileStreamPtr(nullptr, [](std::istream* stream) { delete stream; });
        }
        const std::string data((std::istreambuf_iterator<char>(*bytes)), std::istreambuf_iterator<char>());
        auto text = nb::steal(PyUnicode_Decode(data.data(), static_cast<Py_ssize_t>(data.size()), m_encoding.c_str(), "strict"));
        if (!text)
        {
            throw nb::python_error();
        }
        return TextStream(Utf8(text));
    }
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string& /*name*/) const override { return NoWideStream(); }
    [[nodiscard]] std::optional<TimePoint> GetLastModificationDate(const std::string& name) const override { return m_fs.GetLastModificationDate(name); }
    [[nodiscard]] bool IsEqual(const jinja2::IComparable& other) const override
    {
        const auto* handler = dynamic_cast<const DecodingHandler*>(&other);
        return handler && handler->m_fs.IsEqual(m_fs) && handler->m_encoding == m_encoding;
    }

private:
    jinja2::ConfinedFileSystem m_fs;
    std::string m_encoding;
};

// Any other loader: loader.get_source(environment, name) -> (source, filename, uptodate)
class SourceHandler final : public jinja2::IFilesystemHandler
{
public:
    SourceHandler(PyObject* loader, PyObject* environment)
        : m_loader(loader)
        , m_environment(environment)
    {
    }

    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string& name) const override
    {
        const auto key = MakeStr(name);
        PyObject* result = PyObject_CallMethod(m_loader, "get_source", "OO", m_environment, key.ptr());
        if (!result)
        {
            if (PyRef missed; PendingIsNotFound(&missed))
            {
                // Kept for the error: Jinja2 raises the loader's own TemplateNotFound
                m_missedName = name;
                m_missed = std::move(missed);
                return CharFileStreamPtr(nullptr, [](std::istream* stream) { delete stream; });
            }
            throw nb::python_error();
        }
        const auto triple = nb::steal(result);
        const auto source = triple[0];
        auto& entry = m_entries[name];
        entry.filename = PyRef::Borrow(nb::object(triple[1]).ptr());
        entry.uptodate = PyRef::Borrow(nb::object(triple[2]).ptr());
        entry.fresh = true;
        return TextStream(Utf8(source));
    }
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string& /*name*/) const override { return NoWideStream(); }
    [[nodiscard]] std::optional<TimePoint> GetLastModificationDate(const std::string& name) const override
    {
        auto found = m_entries.find(name);
        if (found == m_entries.end() || !found->second.uptodate || found->second.uptodate.Get() == Py_None)
        {
            // No uptodate: Jinja2 never reloads it
            return std::nullopt;
        }
        if (found->second.fresh)
        {
            // The engine records the date of a source it has just read, which is current:
            // Jinja2 asks uptodate only to decide on a reload, and not at all without auto_reload
            found->second.fresh = false;
            return m_versions.Current(name, false);
        }
        PyObject* result = PyObject_CallNoArgs(found->second.uptodate.Get());
        if (!result)
        {
            throw nb::python_error();
        }
        const int upToDate = PyObject_IsTrue(result);
        Py_DECREF(result);
        if (upToDate < 0)
        {
            throw nb::python_error();
        }
        return m_versions.Current(name, upToDate == 0);
    }
    [[nodiscard]] bool IsEqual(const jinja2::IComparable& other) const override
    {
        const auto* handler = dynamic_cast<const SourceHandler*>(&other);
        return handler && handler->m_loader == m_loader;
    }

    int Traverse(visitproc visit, void* arg) const
    {
        for (const auto& [name, entry] : m_entries)
        {
            Py_VISIT(entry.filename.Get());
            Py_VISIT(entry.uptodate.Get());
        }
        Py_VISIT(m_missed.Get());
        return 0;
    }
    void Clear() const noexcept
    {
        m_entries.clear();
        m_missed = PyRef();
    }

    // The TemplateNotFound the loader raised when `name` was last not found, or None. It is handed over, not
    // kept: once raised it holds the caller's frames, which a later miss must not keep alive
    void ForgetMissed() const noexcept { m_missed = PyRef(); }
    [[nodiscard]] nb::object Missed(const std::string& name) const
    {
        if (!m_missed || m_missedName != name)
        {
            return nb::none();
        }
        auto missed = nb::borrow(m_missed.Get());
        m_missed = PyRef();
        return missed;
    }

    // The source of `name` for a diagnostic, read again from the loader: None when it fails
    [[nodiscard]] nb::object Source(const std::string& name) const
    {
        try
        {
            const auto triple = nb::steal(PyObject_CallMethod(m_loader, "get_source", "OO", m_environment, MakeStr(name).ptr()));
            if (!triple)
            {
                throw nb::python_error();
            }
            return triple[0];
        }
        catch (const nb::python_error&) // NOLINT(bugprone-empty-catch): the source is optional
        {
        }
        return nb::none();
    }
    [[nodiscard]] nb::object Filename(const std::string& name) const
    {
        auto found = m_entries.find(name);
        return found != m_entries.end() && found->second.filename ? nb::borrow(found->second.filename.Get()) : nb::none();
    }

private:
    struct Entry
    {
        PyRef filename;
        PyRef uptodate;
        // Read by OpenStream, and the engine has not asked for its date yet
        bool fresh = false;
    };
    PyObject* m_loader;
    PyObject* m_environment;
    mutable std::unordered_map<std::string, Entry> m_entries;
    // The last name not found and the loader's exception for it (renders hold the GIL)
    mutable std::string m_missedName;
    mutable PyRef m_missed;
    Versions m_versions;
};

class DictLoaderImpl final : public Loader
{
public:
    explicit DictLoaderImpl(PyObject* loader)
        : m_handler(std::make_shared<DictHandler>(loader))
    {
    }
    void Install(jinja2::TemplateEnv& env) const override { env.AddFilesystemHandler(std::string(), m_handler); }
    [[nodiscard]] ErrorSource SourceOf(const std::string& name, bool withSource) const override
    {
        ErrorSource source;
        source.name = MakeStr(name);
        try
        {
            if (auto text = withSource ? m_handler->Read(name) : std::nullopt)
            {
                source.source = MakeStr(*text);
            }
        }
        catch (const nb::python_error&) // NOLINT(bugprone-empty-catch): the source is optional
        {
        }
        return source;
    }

private:
    std::shared_ptr<DictHandler> m_handler;
};

class FileSystemLoaderImpl final : public Loader
{
public:
    FileSystemLoaderImpl(std::vector<std::string> paths, std::string encoding, bool utf8)
        : m_paths(std::move(paths))
        , m_encoding(std::move(encoding))
    {
        for (const auto& path : m_paths)
        {
            m_readers.push_back(std::make_shared<jinja2::ConfinedFileSystem>(path));
            if (utf8)
            {
                m_handlers.push_back(std::make_shared<jinja2::ConfinedFileSystem>(path));
            }
            else
            {
                m_handlers.push_back(std::make_shared<DecodingHandler>(path, m_encoding));
            }
        }
    }
    void Install(jinja2::TemplateEnv& env) const override
    {
        for (const auto& handler : m_handlers)
        {
            env.AddFilesystemHandler(std::string(), handler);
        }
    }
    [[nodiscard]] ErrorSource SourceOf(const std::string& name, bool withSource) const override
    {
        ErrorSource source;
        source.name = MakeStr(name);
        // The first directory the engine reads the file from: the same confined reads as the
        // loading, so a link out of the directory never supplies the diagnostic's source
        const auto os = nb::module_::import_("os.path");
        for (std::size_t i = 0; i < m_paths.size(); ++i)
        {
            auto bytes = m_readers[i]->OpenByteStream(name);
            if (!bytes)
            {
                continue;
            }
            source.filename = os.attr("join")(MakeStr(m_paths[i]), MakeStr(name));
            if (!withSource)
            {
                break;
            }
            const std::string data((std::istreambuf_iterator<char>(*bytes)), std::istreambuf_iterator<char>());
            PyObject* text = PyUnicode_Decode(data.data(), static_cast<Py_ssize_t>(data.size()), m_encoding.c_str(), "strict");
            if (text)
            {
                source.source = nb::steal(text);
            }
            else
            {
                PyErr_Clear();
            }
            break;
        }
        return source;
    }

private:
    std::vector<std::string> m_paths;
    std::string m_encoding;
    std::vector<jinja2::FilesystemHandlerPtr> m_handlers;
    // One confined reader per directory, for the diagnostic source
    std::vector<std::shared_ptr<jinja2::ConfinedFileSystem>> m_readers;
};

class SourceLoaderImpl final : public Loader
{
public:
    SourceLoaderImpl(PyObject* loader, PyObject* environment)
        : m_handler(std::make_shared<SourceHandler>(loader, environment))
    {
    }
    void Install(jinja2::TemplateEnv& env) const override { env.AddFilesystemHandler(std::string(), m_handler); }
    int Traverse(visitproc visit, void* arg) const override { return m_handler->Traverse(visit, arg); }
    void Clear() const noexcept override { m_handler->Clear(); }
    [[nodiscard]] ErrorSource SourceOf(const std::string& name, bool withSource) const override
    {
        ErrorSource source;
        source.name = MakeStr(name);
        source.filename = m_handler->Filename(name);
        if (withSource)
        {
            source.source = m_handler->Source(name);
        }
        return source;
    }
    [[nodiscard]] nb::object NotFound(const std::string& name) const override { return m_handler->Missed(name); }
    void ForgetMissed() const noexcept override { m_handler->ForgetMissed(); }

private:
    std::shared_ptr<SourceHandler> m_handler;
};

bool IsLoaderClass(nb::handle loader, std::string_view name)
{
    auto* type = reinterpret_cast<PyObject*>(Py_TYPE(loader.ptr()));
    return IsJinjaClass(type, name);
}
} // namespace

LoaderPtr MakeLoader(nb::handle loader, nb::handle environment)
{
    if (loader.is_none())
    {
        return nullptr;
    }
    if (IsLoaderClass(loader, "DictLoader"))
    {
        return std::make_shared<DictLoaderImpl>(loader.ptr());
    }
    if (IsLoaderClass(loader, "FileSystemLoader"))
    {
        const auto os = nb::module_::import_("os");
        std::vector<std::string> paths;
        for (auto path : loader.attr("searchpath"))
        {
            paths.push_back(Utf8(os.attr("fspath")(path)));
        }
        const auto encoding = nb::object(loader.attr("encoding"));
        const auto codec = nb::module_::import_("codecs").attr("lookup")(encoding);
        const bool utf8 = Utf8(codec.attr("name")) == "utf-8";
        return std::make_shared<FileSystemLoaderImpl>(std::move(paths), Utf8(encoding), utf8);
    }
    if (!nb::hasattr(loader, "get_source"))
    {
        RaiseTypeError("the loader has no get_source method");
    }
    return std::make_shared<SourceLoaderImpl>(loader.ptr(), environment.ptr());
}
} // namespace jinja2cpp_py

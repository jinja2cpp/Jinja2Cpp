#include <jinja2cpp/template_env.h>

#include "big_int.h"
#include "recursion_guard.h"
#include "error_boundary.h"
#include "load_settings.h"
#include "make_unexpected.h"
#include "sandbox_policy.h"
#include "shared_owner.h"
#include "dialect.h"
#include "template_env_impl.h"
#include "thread_annotations.h"

#include <jinja2cpp/big_int_backend.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace
{
// Every Load compares the settings (TemplateEnvImpl::GetLoadSettings), and their strings are a
// few characters long: compared in place rather than through a memcmp call each (0146)
bool IsSameText(const std::string& lhs, const std::string& rhs)
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    const char* const left = lhs.data();
    const char* const right = rhs.data();
    for (std::size_t idx = 0; idx != lhs.size(); ++idx)
    {
        if (left[idx] != right[idx])
        {
            return false;
        }
    }
    return true;
}
// Word by word without a branch or a memcmp call: compiled to a few vector compares
template<typename T>
bool IsSameBytes(const T& lhs, const T& rhs)
{
    static_assert(std::has_unique_object_representations_v<T> && sizeof(T) % sizeof(std::uint64_t) == 0);
    const auto* const left = reinterpret_cast<const unsigned char*>(&lhs);  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto* const right = reinterpret_cast<const unsigned char*>(&rhs); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    std::uint64_t diff = 0;
    for (std::size_t offset = 0; offset != sizeof(T); offset += sizeof(std::uint64_t))
    {
        std::uint64_t leftWord = 0;
        std::uint64_t rightWord = 0;
        std::memcpy(&leftWord, left + offset, sizeof(leftWord));
        std::memcpy(&rightWord, right + offset, sizeof(rightWord));
        diff |= leftWord ^ rightWord;
    }
    return diff == 0;
}
} // namespace

bool operator==(const Settings& lhs, const Settings& rhs)
{
    // Structured bindings name every field: a field added to Settings or Settings::Extensions stops this compiling
    // until it is compared here too
    const auto& [lTrim, lLstrip, lCacheSize, lAutoReload, lExt, lMetaType, lKeepNl, lNlSeq, lVarStart, lVarEnd, lBlockStart, lBlockEnd, lCommentStart,
                 lCommentEnd, lLineStmt, lLineComment, lFinalize, lAutoescape, lAutoescapeSelector, lAutoescapeMode, lPromptEscaper, lTokenizer, lUndefined, lLookup, lCallablesAsFilters, lImmutable, lSandbox, lLimits] = lhs;
    const auto& [rTrim, rLstrip, rCacheSize, rAutoReload, rExt, rMetaType, rKeepNl, rNlSeq, rVarStart, rVarEnd, rBlockStart, rBlockEnd, rCommentStart,
                 rCommentEnd, rLineStmt, rLineComment, rFinalize, rAutoescape, rAutoescapeSelector, rAutoescapeMode, rPromptEscaper, rTokenizer, rUndefined, rLookup, rCallablesAsFilters, rImmutable, rSandbox, rLimits] = rhs;
    const auto& [lDo, lLoopControls, lI18n] = lExt;
    const auto& [rDo, rLoopControls, rI18n] = rExt;

    // The flags and numbers first, then the strings
    if (std::tie(lTrim, lLstrip, lCacheSize, lAutoReload, lDo, lLoopControls, lI18n, lKeepNl, lAutoescape, lAutoescapeSelector, lAutoescapeMode, lPromptEscaper, lTokenizer, lUndefined, lLookup, lCallablesAsFilters, lImmutable, lSandbox)
        != std::tie(rTrim, rLstrip, rCacheSize, rAutoReload, rDo, rLoopControls, rI18n, rKeepNl, rAutoescape, rAutoescapeSelector, rAutoescapeMode, rPromptEscaper, rTokenizer, rUndefined, rLookup, rCallablesAsFilters, rImmutable, rSandbox))
    {
        return false;
    }
    // Every Load compares the settings, so the limits compare as bytes: integers without padding, and a field added
    // to Settings::Limits changes the size checked here
    static_assert(sizeof(Settings::Limits) == 80);
    if (!IsSameBytes(lLimits, rLimits))
    {
        return false;
    }
    if (!IsSameText(lMetaType, rMetaType) || !IsSameText(lNlSeq, rNlSeq) || !IsSameText(lVarStart, rVarStart) || !IsSameText(lVarEnd, rVarEnd)
        || !IsSameText(lBlockStart, rBlockStart) || !IsSameText(lBlockEnd, rBlockEnd) || !IsSameText(lCommentStart, rCommentStart)
        || !IsSameText(lCommentEnd, rCommentEnd) || !IsSameText(lLineStmt, rLineStmt) || !IsSameText(lLineComment, rLineComment))
    {
        return false;
    }
    // A default UserCallable still has an identity of its own, so two unset ones are compared by the missing callable
    return lFinalize.callable || rFinalize.callable ? lFinalize.IsEqual(rFinalize) : true;
}

namespace
{
// Not std::tolower: the process locale must not change which names escape (a Turkish one
// does not lower 'I' to 'i')
char AsciiLower(char ch)
{
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}
} // namespace

AutoescapeSelector SelectAutoescape(std::vector<std::string> enabledExtensions, std::vector<std::string> disabledExtensions, bool defaultForString, bool defaultValue)
{
    // As Jinja2 does: ".ext", in lower case, matched against the end of the name in lower case
    auto toSuffixes = [](std::vector<std::string>& extensions) {
        for (auto& ext : extensions)
        {
            ext.erase(0, ext.find_first_not_of('.'));
            std::transform(ext.begin(), ext.end(), ext.begin(), AsciiLower);
            ext.insert(ext.begin(), '.');
        }
    };
    toSuffixes(enabledExtensions);
    toSuffixes(disabledExtensions);
    return [enabled = std::move(enabledExtensions), disabled = std::move(disabledExtensions), defaultForString, defaultValue](const std::string& templateName) {
        if (templateName.empty())
        {
            return defaultForString;
        }
        std::string name = templateName;
        std::transform(name.begin(), name.end(), name.begin(), AsciiLower);
        auto endsWithAny = [&name](const std::vector<std::string>& suffixes) {
            return std::any_of(suffixes.begin(), suffixes.end(), [&name](const std::string& suffix) {
                return name.size() >= suffix.size() && name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
            });
        };
        if (endsWithAny(enabled))
        {
            return true;
        }
        if (endsWithAny(disabled))
        {
            return false;
        }
        return defaultValue;
    };
}

namespace detail
{
namespace
{
bool IsSameHandler(const FilesystemHandlerPtr& lhs, const FilesystemHandlerPtr& rhs)
{
    if (lhs && rhs)
    {
        return lhs->IsEqual(*rhs);
    }
    return !lhs && !rhs;
}

bool IsSameCallables(const TemplateEnvImpl::CallablesMap& lhs, const TemplateEnvImpl::CallablesMap& rhs)
{
    return lhs.size() == rhs.size() && std::all_of(lhs.begin(), lhs.end(), [&rhs](const auto& item) {
               auto p = rhs.find(item.first);
               return p != rhs.end() && item.second.IsEqual(p->second);
           });
}

std::optional<UserCallable> FindCallable(const TemplateEnvImpl::CallablesMap& callables, const std::string& name)
{
    auto p = callables.find(name);
    return p == callables.end() ? std::optional<UserCallable>() : std::optional<UserCallable>(p->second);
}

template<typename CharT>
struct TemplateFunctions;

template<>
struct TemplateFunctions<char>
{
    using ResultType = Result<Template>;
    static Template CreateTemplate(TemplateEnv* env) { return Template(env); }
    static auto LoadFile(const std::string& fileName, const IFilesystemHandler* fs) { return fs->OpenStream(fileName); }
    // The caller holds `guard` around every use of the cache; the analysis does not follow the reference
    static auto& GetCache(TemplateEnvImpl& impl) JINJA2CPP_NO_THREAD_SAFETY_ANALYSIS { return impl.templateCache; }
};

template<>
struct TemplateFunctions<wchar_t>
{
    using ResultType = ResultW<TemplateW>;
    static TemplateW CreateTemplate(TemplateEnv* env) { return TemplateW(env); }
    static auto LoadFile(const std::string& fileName, const IFilesystemHandler* fs) { return fs->OpenWStream(fileName); }
    // The caller holds `guard` around every use of the cache; the analysis does not follow the reference
    static auto& GetCache(TemplateEnvImpl& impl) JINJA2CPP_NO_THREAD_SAFETY_ANALYSIS { return impl.templateWCache; }
};

// Erases the least recently used entries of `cache` until it holds `cacheSize` (Python's
// Environment.cache_size: a negative size never evicts) and moves their templates to `released`,
// for the caller to release after the lock. The scan is linear, but it only runs on a miss, after
// a file was opened and parsed
template<typename Cache, typename TemplateT>
void EvictLeastRecentlyUsed(Cache& cache, int cacheSize, std::vector<TemplateT>& released)
{
    if (cacheSize < 0)
    {
        return;
    }
    while (cache.size() > static_cast<size_t>(cacheSize))
    {
        auto oldest = cache.begin();
        for (auto p = cache.begin(); p != cache.end(); ++p)
        {
            if (p->second.lastUse.load(std::memory_order_relaxed) < oldest->second.lastUse.load(std::memory_order_relaxed))
            {
                oldest = p;
            }
        }
        released.reserve(released.size() + 1); // may throw: nothing is moved or erased yet
        released.push_back(std::move(oldest->second.tpl));
        cache.erase(oldest);
    }
}

} // namespace

bool TemplateEnvImpl::FsHandler::operator==(const FsHandler& rhs) const
{
    return prefix == rhs.prefix && IsSameHandler(handler, rhs.handler);
}

bool TemplateEnvImpl::BaseTemplateInfo::operator==(const BaseTemplateInfo& other) const
{
    return lastModification == other.lastModification && IsSameHandler(handler, other.handler);
}

bool TemplateEnvImpl::IsEqual(const TemplateEnvImpl& other) const
{
    if (this == &other)
    {
        return true;
    }
    const ReaderLockPair lock(guard, other.guard);
    return filesystemHandlers == other.filesystemHandlers && settings == other.settings && GetBigIntBackend() == other.GetBigIntBackend() && GetDialect() == other.GetDialect() && *globalValues == *other.globalValues && IsSameCallables(filters, other.filters) && IsSameCallables(tests, other.tests) && IsSameCallables(translations, other.translations) && templateCache == other.templateCache && templateWCache == other.templateWCache;
}

LoadSettingsPtr TemplateEnvImpl::GetLoadSettings() const
{
    // Comparing is cheaper than copying the settings and rebuilding the delimiters, and catches changes made through
    // the reference TemplateEnv::GetSettings returns
    // A finalize callable edited in place keeps its identity, so it would compare equal: templates of an environment
    // with one get settings of their own
    const WriterLock<Mutex> l(m_loadSettingsGuard);
    if (settings.finalize.callable)
    {
        return std::make_shared<const LoadSettings>(settings, BigIntBackendLocked(), DialectLocked());
    }
    if (!m_loadSettings || m_loadSettings->settings != settings)
    {
        m_loadSettings = std::make_shared<const LoadSettings>(settings, BigIntBackendLocked(), DialectLocked());
    }
    return m_loadSettings;
}

bool TemplateEnvImpl::SetBigIntBackend(const BigIntBackend* backend) noexcept
{
    const WriterLock<Mutex> l(m_loadSettingsGuard);
    if (backend == BigIntBackendLocked())
    {
        return true;
    }
    try
    {
        m_loadSettings = std::make_shared<const LoadSettings>(settings, backend, DialectLocked());
    }
    catch (...)
    {
        return false;
    }
    return true;
}

const BigIntBackend* TemplateEnvImpl::GetBigIntBackend() const noexcept
{
    const WriterLock<Mutex> l(m_loadSettingsGuard);
    return BigIntBackendLocked();
}

void TemplateEnvImpl::SetDialect(Dialect dialect)
{
    const WriterLock<Mutex> l(m_loadSettingsGuard);
    if (dialect == DialectLocked())
    {
        return;
    }
    m_loadSettings = std::make_shared<const LoadSettings>(settings, BigIntBackendLocked(), dialect);
}

Dialect TemplateEnvImpl::GetDialect() const
{
    const WriterLock<Mutex> l(m_loadSettingsGuard);
    return DialectLocked();
}

const LoadSettingsPtr& DefaultLoadSettings()
{
    static const LoadSettingsPtr defaultSettings = std::make_shared<const LoadSettings>(Settings(), GetBuiltinBigIntBackend());
    return defaultSettings;
}

template<typename CharT>
auto TemplateEnvImpl::LoadTemplate(TemplateEnv* env, std::string fileName)
{
    using Functions = TemplateFunctions<CharT>;
    using ResultType = typename Functions::ResultType;
    using ErrorType = typename ResultType::error_type;
    // Filesystem handlers and the template loading are host code and parsing: whatever they throw,
    // except std::bad_alloc, becomes the error (docs/api-2.0.md, 2.1)
    try
    {
        // A loaded template is the environment's (its cache, other renders), not the render's that loads it first
        const UncountedMemoryScope uncounted;
        auto& cache = Functions::GetCache(*this);

        {
            // The freshness check is host code (a handler may load another template), so it runs
            // outside the lock: a nested load takes it exclusively. The entry is copied under
            // the lock
            std::optional<decltype(cache.begin()->second.tpl)> cachedTpl; // empty on a hit: a default template allocates
            FilesystemHandlerPtr cachedHandler;
            std::optional<TimePoint> cachedModification;
            {
                const detail::ReaderLock l(guard);
                auto p = cache.find(fileName);
                if (p != cache.end())
                {
                    if (!settings.autoReload)
                    {
                        p->second.lastUse.store(NewCacheTick(), std::memory_order_relaxed);
                        return ResultType(p->second.tpl);
                    }
                    cachedTpl = p->second.tpl;
                    cachedHandler = p->second.handler;
                    cachedModification = p->second.lastModification;
                }
            }
            if (cachedHandler && cachedTpl)
            {
                auto lastModified = cachedHandler->GetLastModificationDate(fileName);
                if (!lastModified || (cachedModification && lastModified.value() <= cachedModification.value()))
                {
                    const detail::ReaderLock l(guard);
                    auto p = cache.find(fileName);
                    // Still the entry that was checked; if it was replaced meanwhile, the template
                    // just checked is still a valid answer for this call
                    if (p != cache.end() && p->second.tpl == *cachedTpl)
                    {
                        p->second.lastUse.store(NewCacheTick(), std::memory_order_relaxed);
                    }
                    return ResultType(*cachedTpl);
                }
            }
        }

        // Created only on a miss: a new template allocates its environment handle
        auto tpl = Functions::CreateTemplate(env);
        for (auto& fh : filesystemHandlers)
        {
            if (!fh.prefix.empty() && fileName.find(fh.prefix) != 0)
            {
                continue;
            }

            // The date is asked before the text is read: a change during the read then makes the cached date older
            // than the file's, so the next lookup reloads instead of keeping the old text as up to date (0325)
            std::optional<TimePoint> lastModified;
            if (settings.cacheSize != 0)
            {
                lastModified = fh.handler->GetLastModificationDate(fileName);
            }
            auto stream = Functions::LoadFile(fileName, fh.handler.get());
            if (stream)
            {
                auto res = tpl.Load(*stream, fileName);
                if (!res)
                {
                    return ResultType(MakeUnexpected(res.error()));
                }

                if (settings.cacheSize != 0)
                {
                    // Templates this replaces (another thread loaded the same file concurrently, or
                    // a reload) or evicts are released after the lock: one may hold the last handle
                    // to this environment, and ~TemplateEnv takes the same lock
                    std::vector<decltype(tpl)> released;
                    // Reserved before the cache changes: a bad_alloc then leaves no half-made entry
                    released.reserve(1);
                    const detail::WriterLock<detail::SharedMutex> l(guard);
                    if (owner)
                    {
                        auto& cacheEntry = cache[fileName];
                        released.push_back(std::move(cacheEntry.tpl));
                        cacheEntry.tpl = tpl;
                        cacheEntry.handler = fh.handler;
                        cacheEntry.lastModification = lastModified;
                        cacheEntry.lastUse.store(NewCacheTick(), std::memory_order_relaxed);
                        EvictLeastRecentlyUsed(cache, settings.cacheSize, released);
                    }
                }

                return ResultType(tpl);
            }
        }

        typename ErrorType::Data errorData;
        errorData.code = ErrorCode::FileNotFound;
        errorData.srcLoc.col = 1;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.fileName = "";
        errorData.extraParams.push_back(Value(fileName));

        return ResultType(MakeUnexpected(ErrorType(errorData)));
    }
    catch (...)
    {
        return ResultType(MakeUnexpected(CurrentExceptionToError<CharT>(fileName)));
    }
}
} // namespace detail

TemplateEnv::TemplateEnv()
    : m_impl(std::make_shared<detail::TemplateEnvImpl>())
{
    m_impl->owner = this;
}

TemplateEnv::TemplateEnv(std::shared_ptr<detail::TemplateEnvImpl> impl)
    : m_impl(std::move(impl))
{
}

TemplateEnv::~TemplateEnv()
{
    if (!m_impl)
    {
        return;
    }
    // The handles templates keep never own the state, and only the owner changes `owner`
    if (m_impl->owner != this)
    {
        return;
    }
    detail::TemplateEnvImpl::TemplateCache<Template> templateCache;
    detail::TemplateEnvImpl::TemplateCache<TemplateW> templateWCache;
    {
        const detail::WriterLock<detail::SharedMutex> l(m_impl->guard);
        m_impl->owner = nullptr;
        templateCache.swap(m_impl->templateCache);
        templateWCache.swap(m_impl->templateWCache);
    }
    // The cached templates are released here, outside the lock: they hold handles to this state
}

const Settings& TemplateEnv::GetSettings() const
{
    return m_impl->settings;
}

Settings& TemplateEnv::GetSettings()
{
    return m_impl->settings;
}

void TemplateEnv::SetSettings(const Settings& setts)
{
    m_impl->settings = setts;
}

bool TemplateEnv::SetBigIntBackend(const BigIntBackend* backend) noexcept
{
    if (backend && !IsUsableBigIntBackend(*backend))
    {
        return false;
    }
    return m_impl->SetBigIntBackend(backend);
}

const BigIntBackend* TemplateEnv::GetBigIntBackend() const noexcept
{
    return m_impl->GetBigIntBackend();
}

void TemplateEnv::AddFilesystemHandler(std::string prefix, FilesystemHandlerPtr h)
{
    m_impl->filesystemHandlers.push_back(detail::TemplateEnvImpl::FsHandler{ std::move(prefix), std::move(h) });
}

void TemplateEnv::AddFilesystemHandler(std::string prefix, IFilesystemHandler& h)
{
    AddFilesystemHandler(std::move(prefix), std::shared_ptr<IFilesystemHandler>(&h, [](auto*) {}));
}

Result<Template> TemplateEnv::LoadTemplate(std::string fileName)
{
    return m_impl->LoadTemplate<char>(this, std::move(fileName));
}

ResultW<TemplateW> TemplateEnv::LoadTemplateW(std::string fileName)
{
    return m_impl->LoadTemplate<wchar_t>(this, std::move(fileName));
}

Result<Template> TemplateEnv::FromString(std::string_view source, std::string name)
{
    Template tpl(this);
    auto res = tpl.Load(source, std::move(name));
    if (!res)
    {
        return MakeUnexpected(res.error());
    }
    return tpl;
}

ResultW<TemplateW> TemplateEnv::FromString(std::wstring_view source, std::string name)
{
    TemplateW tpl(this);
    auto res = tpl.Load(source, std::move(name));
    if (!res)
    {
        return MakeUnexpected(res.error());
    }
    return tpl;
}

uint64_t detail::TemplateEnvImpl::NewCacheTick() noexcept
{
    static std::atomic<uint64_t> lastTick{ 0 };
    return lastTick.fetch_add(1, std::memory_order_relaxed) + 1;
}

uint64_t detail::TemplateEnvImpl::NewGlobalsGeneration()
{
    static std::atomic<uint64_t> lastGeneration{ 0 };
    return lastGeneration.fetch_add(1, std::memory_order_relaxed) + 1;
}

namespace
{
// Changes the globals of `impl` with `fn`, under its lock
template<typename Fn>
void ChangeGlobals(detail::TemplateEnvImpl& impl, const Fn& fn)
{
    const detail::WriterLock<detail::SharedMutex> l(impl.guard);
    // A render may still hold the map: it gets a new one. One whose thread let go of it has its
    // reads ordered before the change (docs/tasks/0339)
    if (!IsSoleOwner(impl.globalValues))
    {
        impl.globalValues = std::make_shared<ValuesMap>(*impl.globalValues);
    }
    fn(*impl.globalValues);
    impl.globalsGeneration.store(detail::TemplateEnvImpl::NewGlobalsGeneration(), std::memory_order_release);
}
} // namespace

void TemplateEnv::AddGlobal(std::string name, Value val)
{
    ChangeGlobals(*m_impl, [&name, &val](ValuesMap& globals) { globals[std::move(name)] = std::move(val); });
}

void TemplateEnv::RemoveGlobal(const std::string& name)
{
    ChangeGlobals(*m_impl, [&name](ValuesMap& globals) { globals.erase(name); });
}

void TemplateEnv::AddFilter(std::string name, UserCallable filter)
{
    const detail::WriterLock<detail::SharedMutex> l(m_impl->guard);
    m_impl->filters[std::move(name)] = std::move(filter);
}

void TemplateEnv::RemoveFilter(const std::string& name)
{
    const detail::WriterLock<detail::SharedMutex> l(m_impl->guard);
    m_impl->filters.erase(name);
}

void TemplateEnv::AddTest(std::string name, UserCallable test)
{
    const detail::WriterLock<detail::SharedMutex> l(m_impl->guard);
    m_impl->tests[std::move(name)] = std::move(test);
}

void TemplateEnv::RemoveTest(const std::string& name)
{
    const detail::WriterLock<detail::SharedMutex> l(m_impl->guard);
    m_impl->tests.erase(name);
}

std::optional<UserCallable> TemplateEnv::FindFilter(const std::string& name) const
{
    const detail::ReaderLock l(m_impl->guard);
    return detail::FindCallable(m_impl->filters, name);
}

std::optional<UserCallable> TemplateEnv::FindTest(const std::string& name) const
{
    const detail::ReaderLock l(m_impl->guard);
    return detail::FindCallable(m_impl->tests, name);
}

void TemplateEnv::InstallGettextCallables(UserCallable gettext, UserCallable ngettext, UserCallable pgettext, UserCallable npgettext)
{
    const detail::WriterLock<detail::SharedMutex> l(m_impl->guard);
    m_impl->translations.clear();
    // A loop rather than a lambda: the analysis checks a lambda as a function of its own, which holds no lock
    const std::array<std::pair<const char*, UserCallable*>, 4> callables{ { { "gettext", &gettext }, { "ngettext", &ngettext }, { "pgettext", &pgettext }, { "npgettext", &npgettext } } };
    for (const auto& [name, fn] : callables)
    {
        if (fn->callable)
        {
            m_impl->translations[name] = std::move(*fn);
        }
    }
}

std::optional<UserCallable> TemplateEnv::FindGettextCallable(const std::string& name) const
{
    const detail::ReaderLock l(m_impl->guard);
    return detail::FindCallable(m_impl->translations, name);
}

void TemplateEnv::ApplyGlobals(const std::function<void(const ValuesMap&)>& fn) const
{
    const detail::ReaderLock l(m_impl->guard);
    fn(*m_impl->globalValues);
}

} // namespace v2
} // namespace jinja2

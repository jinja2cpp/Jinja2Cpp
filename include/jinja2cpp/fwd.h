#ifndef JINJA2CPP_FWD_H
#define JINJA2CPP_FWD_H

// Forward declarations of the public types, for headers that only name them. Declare
// Jinja2C++ types through this header rather than by hand: they live in an inline ABI
// namespace (docs/tasks/0076), which a hand-written `namespace jinja2 { class Value; }`
// does not reopen. The default template arguments of the public templates are given here
// and only here.

#include <memory>
#include <vector>

namespace jinja2
{
inline namespace v2
{
class Value;
class Exception;
class GenericList;
class GenericMap;
class TemplateEnv;

struct NoneValue;
struct BigIntBackend;
struct ValuesMap;
struct UserCallable;
struct IComparable;
struct IListItemAccessor;
struct IIndexBasedAccessor;
struct IListEnumerator;
struct IMapItemAccessor;
class IFilesystemHandler;
class MemoryFileSystem;
class RealFileSystem;
class ConfinedFileSystem;
struct Settings;
struct SourceLocation;

template<typename CharT>
struct MetadataInfo;
template<typename CharT>
class BasicTemplate;
template<typename CharT>
class BasicErrorInfo;

namespace detail
{
template<typename T, typename Tag = void>
struct Reflector;
} // namespace detail

template<typename T, typename Tag = void>
struct Reflector;
template<typename T, typename = void>
struct TypeReflection;

enum class ErrorCode;
enum class UndefinedPolicy;
enum class TemplateLookup;

using Template = BasicTemplate<char>;
using TemplateW = BasicTemplate<wchar_t>;
using ErrorInfo = BasicErrorInfo<char>;
using ErrorInfoW = BasicErrorInfo<wchar_t>;
using ValuesList = std::vector<Value>;
using FilesystemHandlerPtr = std::shared_ptr<IFilesystemHandler>;
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_FWD_H

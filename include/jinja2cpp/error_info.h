#ifndef JINJA2CPP_ERROR_INFO_H
#define JINJA2CPP_ERROR_INFO_H

#include "fwd.h" // IWYU pragma: export
#include "config.h"
#include "exception.h" // IWYU pragma: export
#include "value.h"

#include <atomic>
#include <exception>
#include <iostream>
#include <string>
#include <system_error>
#include <type_traits> // IWYU pragma: keep (public header: user code may rely on it)
#include <vector>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
/*!
 * \brief Type of the error
 *
 * Every enumerator has an explicit value: codes are logged and persisted by users, so a value never changes and new
 * codes are only appended (parse errors from 1001 on, the rest below 1000).
 */
enum class ErrorCode
{
    Unspecified = 0,               //!< Error is unspecified
    UnexpectedException = 1,       //!< An exception the engine did not raise and could not attribute. ExtraParams[0] contains `what()` string of the exception, GetException() the exception
    YetUnsupported = 2,            //!< Feature of the jinja2 specification which yet not supported
    FileNotFound = 3,              //!< Requested file was not found. ExtraParams[0] contains name of the file
    ExtensionDisabled = 4,         //!< Particular jinja2 extension disabled in the settings
    TemplateEnvAbsent = 5,         //!< Template uses `extend`, `import`, `from` or `include` features but it's loaded without the template environment set
    TemplateNotFound = 6,          //!< Template with the specified name was not found. ExtraParams[0] contains name of the file
    TemplateNotParsed = 7,         //!< Template was not parsed
    InvalidValueType = 8,          //!< Invalid type of the value in the particular context
    InvalidTemplateName = 9,       //!< Invalid name of the template. ExtraParams[0] contains the name
    MetadataParseError = 10,       //!< Template metadata (`{% meta %}` block) could not be parsed. ExtraParams[0] contains the parser message
    UndefinedError = 11,           //!< An undefined value was used in a way its policy (Settings::undefinedPolicy) forbids. ExtraParams[0] contains the message
    RecursionLimitExceeded = 12,   //!< Expressions nest, or macros, includes or recursive loops recurse, deeper than Jinja2C++ allows (Python's RecursionError)
    UnknownFilter = 13,            //!< The template names a filter that is neither built in nor added to the environment (Jinja2's TemplateAssertionError). ExtraParams[0] contains the name
    UnknownTest = 14,              //!< The template names a test that is neither built in nor added to the environment (Jinja2's TemplateAssertionError). ExtraParams[0] contains the name
    UserCallableException = 15,    //!< A user callable (context or global value, filter, test, finalize, gettext) threw. ExtraParams[0] contains `what()`, GetException() the exception
    IntegerOutOfRange = 16,        //!< An integer is outside the `int64_t` range: a \ref BigInt the template uses (until docs/tasks/0270) or template arithmetic. ExtraParams[0] contains the message
    TemplateRuntimeError = 17,     //!< An operation of the template failed (Python's TypeError, ValueError, ...). ExtraParams[0] contains the message
    ResourceLimitExceeded = 18,    //!< A size or depth limit, or a host's budget, was exceeded. ExtraParams[0] contains the message, ExtraParams[1] (when present) the limit's name
    ExpectedStringLiteral = 1001,  //!< String literal expected
    ExpectedIdentifier = 1002,     //!< Identifier expected
    ExpectedSquareBracket = 1003,  //!< ']' expected
    ExpectedRoundBracket = 1004,   //!< ')' expected
    ExpectedCurlyBracket = 1005,   //!< '}' expected
    ExpectedToken = 1006,          //!< Specific token(s) expected. ExtraParams[0] contains the actual token, rest of ExtraParams contain set of expected tokens
    ExpectedExpression = 1007,     //!< Expression expected
    ExpectedEndOfStatement = 1008, //!< End of statement expected. ExtraParams[0] contains the expected end of statement tag
    ExpectedRawEnd = 1009,         //!< {% endraw %} expected
    ExpectedMetaEnd = 1010,        //!< {% endmeta %} expected
    UnexpectedToken = 1011,        //!< Unexpected token. ExtraParams[0] contains the invalid token
    UnexpectedStatement = 1012,    //!< Unexpected statement. ExtraParams[0] contains the invalid statement tag
    UnexpectedCommentBegin = 1013, //!< Unexpected comment block begin (`{#`)
    UnexpectedCommentEnd = 1014,   //!< Unexpected comment block end (`#}`)
    UnexpectedExprBegin = 1015,    //!< Unexpected expression block begin (`{{`)
    UnexpectedExprEnd = 1016,      //!< Unexpected expression block end (`}}`)
    UnexpectedStmtBegin = 1017,    //!< Unexpected statement block begin (`{%`)
    UnexpectedStmtEnd = 1018,      //!< Unexpected statement block end (`%}`)
    UnexpectedRawBegin = 1019,     //!< Unexpected raw block begin {% raw %}
    UnexpectedRawEnd = 1020,       //!< Unexpected raw block end {% endraw %}
    UnexpectedMetaBegin = 1021,    //!< Unexpected meta block begin {% meta %}
    UnexpectedMetaEnd = 1022,      //!< Unexpected meta block end {% endmeta %}
};

//! The `std::error_category` of \ref ErrorCode: `std::error_code ec = ErrorCode::TemplateNotFound;` compares equal to
//! `std::errc::no_such_file_or_directory`
JINJA2CPP_EXPORT const std::error_category& error_category() noexcept; // NOLINT(readability-identifier-naming): spelled as std::generic_category

//! Makes a `std::error_code` of the \ref error_category(); found by argument-dependent lookup
inline std::error_code make_error_code(ErrorCode code) noexcept // NOLINT(readability-identifier-naming): the name std::error_code looks up
{
    return { static_cast<int>(code), error_category() };
}

/*!
 * \brief Information about the source location of the error
 */
struct SourceLocation
{
    //! Name of the file
    std::string fileName;
    //! Line number (1-based)
    unsigned line = 0;
    //! Column number (1-based)
    unsigned col = 0;
};

/*!
 * \brief Detailed information about the parse-time or render-time error
 *
 * If template parsing or rendering fails the detailed error information is provided. Exact specialization of BasicErrorInfo is an object which contains
 * this information. Type of specialization depends on type of the template object: \ref ErrorInfo for \ref Template and \ref ErrorInfoW for \ref TemplateW.
 *
 * Detailed information about an error contains:
 * - Error code
 * - Error location
 * - Other locations related to the error
 * - Description of the location
 * - Extra parameters of the error
 *
 * @tparam CharT Character type which was used in template parser
 */
template<typename CharT>
class BasicErrorInfo
{
public:
    struct Data
    {
        ErrorCode code = ErrorCode::Unspecified;
        SourceLocation srcLoc;
        std::vector<SourceLocation> relatedLocs;
        std::vector<Value> extraParams;
        std::basic_string<CharT> locationDescr;
        //! The exception the error began as, when the engine did not raise it (docs/api-2.0.md, 2.1)
        std::exception_ptr exception;
    };

    //! Default constructor
    BasicErrorInfo() noexcept = default;
    //! Initializing constructor from error description
    explicit BasicErrorInfo(Data data)
        : m_payload(new Payload(std::move(data)))
    {}

    //! Copy constructor: the copy shares the description
    BasicErrorInfo(const BasicErrorInfo<CharT>& other) noexcept
        : m_payload(other.m_payload)
    {
        if (m_payload)
        {
            m_payload->refs.fetch_add(1, std::memory_order_relaxed);
        }
    }
    //! Move constructor
    BasicErrorInfo(BasicErrorInfo<CharT>&& other) noexcept
        : m_payload(other.m_payload)
    {
        other.m_payload = nullptr;
    }

    //! Destructor
    ~BasicErrorInfo() noexcept
    {
        if (m_payload && m_payload->refs.fetch_sub(1, std::memory_order_acq_rel) == 1)
        {
            delete m_payload;
        }
    }

    //! Copy-assignment operator
    BasicErrorInfo& operator=(const BasicErrorInfo<CharT>& other) noexcept
    {
        BasicErrorInfo copy(other);
        copy.Swap(*this);
        return *this;
    }
    //! Move-assignment operator
    BasicErrorInfo& operator=(BasicErrorInfo<CharT>&& other) noexcept
    {
        BasicErrorInfo(std::move(other)).Swap(*this);
        return *this;
    }

    //! Return code of the error
    [[nodiscard]] ErrorCode GetCode() const noexcept
    {
        return m_payload ? m_payload->data.code : ErrorCode::Unspecified;
    }

    //! Return error location in the template file
    [[nodiscard]] const SourceLocation& GetErrorLocation() const noexcept
    {
        return GetData().srcLoc;
    }

    //! Return locations, related to the main error location
    [[nodiscard]] const std::vector<SourceLocation>& GetRelatedLocations() const noexcept
    {
        return GetData().relatedLocs;
    }

    /*!
     * \brief Return location description
     *
     * Return string of two lines. First line is contents of the line with error. Second highlight the exact position within line. For instance:
     * ```
     * {% for i in range(10) endfor%}
     *                    ---^-------
     * ```
     *
     * @return Location description
     */
    [[nodiscard]] const std::basic_string<CharT>& GetLocationDescr() const noexcept
    {
        return GetData().locationDescr;
    }

    /*!
     * \brief Return extra params of the error
     *
     * Extra params is a additional details assiciated with the error. For instance, name of the file which wasn't opened
     *
     * @return Vector with extra error params
     */
    [[nodiscard]] const std::vector<Value>& GetExtraParams() const noexcept { return GetData().extraParams; }

    /*!
     * \brief Return the exception the error began as
     *
     * Set if and only if the failure began as an exception the engine did not raise: one thrown by a user callable,
     * reflector, accessor or filesystem handler. `std::rethrow_exception(error.GetException())` raises it again, as
     * Python Jinja2 would. Copies of the error share the exception object, which lives in the module that threw it:
     * the error must not outlive that module.
     *
     * @return The exception, or a null `std::exception_ptr`
     */
    [[nodiscard]] const std::exception_ptr& GetException() const noexcept { return GetData().exception; }

    //! Convert error to the detailed string representation
    [[nodiscard]] JINJA2CPP_EXPORT std::basic_string<CharT> ToString() const;

private:
    // One immutable description, shared by the copies: an error is one pointer wide and copies without allocating
    struct Payload
    {
        explicit Payload(Data d)
            : data(std::move(d))
        {
        }
        std::atomic<long> refs{ 1 };
        const Data data;
    };

    [[nodiscard]] const Data& GetData() const noexcept
    {
        static const Data empty;
        return m_payload ? m_payload->data : empty;
    }

    void Swap(BasicErrorInfo<CharT>& other) noexcept
    {
        auto* payload = m_payload;
        m_payload = other.m_payload;
        other.m_payload = payload;
    }

    Payload* m_payload = nullptr;
};

using ErrorInfo = BasicErrorInfo<char>;
using ErrorInfoW = BasicErrorInfo<wchar_t>;

// 1.x names, kept until 3.0 (docs/api-2.0.md, 5.1)
template<typename CharT>
using ErrorInfoTpl JINJA2CPP_DEPRECATED("jinja2cpp-2: BasicErrorInfo") = BasicErrorInfo<CharT>;

JINJA2CPP_EXPORT std::ostream& operator<<(std::ostream& os, const ErrorInfo& res);
JINJA2CPP_EXPORT std::wostream& operator<<(std::wostream& os, const ErrorInfoW& res);
} // namespace jinja2

namespace std
{
template<>
struct is_error_code_enum<jinja2::ErrorCode> : true_type
{
};
} // namespace std

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_ERROR_INFO_H

#ifndef JINJA2CPP_ERROR_INFO_H
#define JINJA2CPP_ERROR_INFO_H

#include "config.h"
#include "value.h"

#include <iostream>
#include <type_traits>
#include <vector>

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
    UnexpectedException = 1,       //!< Generic exception occurred during template parsing or execution. ExtraParams[0] contains `what()` string of the exception
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
    };

    //! Default constructor
    BasicErrorInfo() = default;
    //! Initializing constructor from error description
    explicit BasicErrorInfo(Data data)
        : m_errorData(std::move(data))
    {}

    //! Copy constructor
    BasicErrorInfo(const BasicErrorInfo<CharT>&) = default;
    //! Move constructor
    BasicErrorInfo(BasicErrorInfo<CharT>&& val) noexcept
        : m_errorData(std::move(val.m_errorData))
    {}

    //! Destructor
    ~BasicErrorInfo() noexcept = default;

    //! Copy-assignment operator
    BasicErrorInfo& operator=(const BasicErrorInfo<CharT>&) = default;
    //! Move-assignment operator
    BasicErrorInfo& operator=(BasicErrorInfo<CharT>&& val) noexcept
    {
        if (this == &val)
        {
            return *this;
        }

        std::swap(m_errorData.code, val.m_errorData.code);
        std::swap(m_errorData.srcLoc, val.m_errorData.srcLoc);
        std::swap(m_errorData.relatedLocs, val.m_errorData.relatedLocs);
        std::swap(m_errorData.extraParams, val.m_errorData.extraParams);
        std::swap(m_errorData.locationDescr, val.m_errorData.locationDescr);

        return *this;
    }

    //! Return code of the error
    [[nodiscard]] ErrorCode GetCode() const
    {
        return m_errorData.code;
    }

    //! Return error location in the template file
    [[nodiscard]] auto& GetErrorLocation() const
    {
        return m_errorData.srcLoc;
    }

    //! Return locations, related to the main error location
    [[nodiscard]] auto& GetRelatedLocations() const
    {
        return m_errorData.relatedLocs;
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
    [[nodiscard]] const std::basic_string<CharT>& GetLocationDescr() const
    {
        return m_errorData.locationDescr;
    }

    /*!
     * \brief Return extra params of the error
     *
     * Extra params is a additional details assiciated with the error. For instance, name of the file which wasn't opened
     *
     * @return Vector with extra error params
     */
    [[nodiscard]] auto& GetExtraParams() const { return m_errorData.extraParams; }

    //! Convert error to the detailed string representation
    JINJA2CPP_EXPORT [[nodiscard]] std::basic_string<CharT> ToString() const;

private:
    Data m_errorData;
};

using ErrorInfo = BasicErrorInfo<char>;
using ErrorInfoW = BasicErrorInfo<wchar_t>;

// 1.x names, kept until 3.0 (docs/api-2.0.md, 5.1)
template<typename CharT>
using ErrorInfoTpl [[deprecated("jinja2cpp-2: use BasicErrorInfo")]] = BasicErrorInfo<CharT>;

JINJA2CPP_EXPORT std::ostream& operator<<(std::ostream& os, const ErrorInfo& res);
JINJA2CPP_EXPORT std::wostream& operator<<(std::wostream& os, const ErrorInfoW& res);
} // namespace jinja2

#endif // JINJA2CPP_ERROR_INFO_H

#ifndef JINJA2CPP_EXCEPTION_H
#define JINJA2CPP_EXCEPTION_H

#include "config.h"

#include <stdexcept>
#include <string>

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_BEGIN

namespace jinja2
{
inline namespace v2
{
enum class ErrorCode;

/*!
 * \brief The exception type of Jinja2C++ (docs/api-2.0.md, 2.1)
 *
 * Host code (user callables, reflectors, accessors, filesystem handlers, allocators) throws it to report a specific
 * \ref ErrorCode: Load and Render then return that code in their `Result`, with the exception kept in
 * `ErrorInfo::GetException()`.
 *
 * It also records the C++ origin of the throw: the file and line of the expression that constructed it.
 */
class JINJA2CPP_EXPORT Exception : public std::runtime_error
{
public:
    /*!
     * \brief Creates the exception
     *
     * @param code Error code reported for this exception
     * @param message Text returned by `what()`
     * @param file C++ source file of the throw; filled in at the call site
     * @param line Line of the throw in `file`; filled in at the call site
     */
    Exception(ErrorCode code, const std::string& message, const char* file = __builtin_FILE(), unsigned line = __builtin_LINE())
        : std::runtime_error(message)
        , m_code(code)
        , m_file(file)
        , m_line(line)
    {
    }
    //! \copydoc Exception(ErrorCode, const std::string&, const char*, unsigned)
    Exception(ErrorCode code, const char* message, const char* file = __builtin_FILE(), unsigned line = __builtin_LINE())
        : std::runtime_error(message)
        , m_code(code)
        , m_file(file)
        , m_line(line)
    {
    }
    Exception(const Exception&) noexcept = default;
    Exception& operator=(const Exception&) noexcept = default;
    Exception(Exception&&) noexcept = default;
    Exception& operator=(Exception&&) noexcept = default;
    // Out of line: the key function, so that the type has one typeinfo across shared libraries
    ~Exception() override;

    //! Error code reported for this exception
    [[nodiscard]] ErrorCode Code() const noexcept { return m_code; }
    //! C++ source file of the throw
    [[nodiscard]] const char* File() const noexcept { return m_file; }
    //! Line of the throw in \ref File()
    [[nodiscard]] unsigned Line() const noexcept { return m_line; }

private:
    ErrorCode m_code;
    const char* m_file;
    unsigned m_line;
};
} // namespace v2
} // namespace jinja2

JINJA2CPP_SUPPRESS_DLL_INTERFACE_WARNINGS_END

#endif // JINJA2CPP_EXCEPTION_H

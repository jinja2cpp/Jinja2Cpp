#ifndef JINJA2CPP_SRC_ERROR_BOUNDARY_H
#define JINJA2CPP_SRC_ERROR_BOUNDARY_H

#include "engine_errors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>

#include <exception>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
// The error a public Load or Render boundary returns for the exception being handled (docs/api-2.0.md,
// 2.1): its code, its what() and, when the engine did not raise it, the exception itself.
// Call it from a catch handler only. std::bad_alloc goes on unchanged, before anything allocates.
template<typename CharT>
BasicErrorInfo<CharT> CurrentExceptionToError(const std::string& fileName)
{
    ErrorCode code = ErrorCode::UnexpectedException;
    const char* message = "unknown exception";
    // Kept only for an exception raised outside the engine (docs/api-2.0.md, 2.1)
    bool fromOutside = true;
    // The Settings::Limits field a limit names, if any
    const char* limitField = nullptr;
    try
    {
        throw;
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (const Exception& ex)
    {
        code = ex.Code();
        message = ex.what();
    }
    catch (const ResourceLimitError& ex)
    {
        code = ErrorCode::ResourceLimitExceeded;
        message = ex.what();
        limitField = ex.Field();
        fromOutside = false;
    }
    catch (const IntegerOutOfRangeError& ex)
    {
        code = ErrorCode::IntegerOutOfRange;
        message = ex.what();
        fromOutside = false;
    }
    catch (const std::length_error& ex)
    {
        // A string or container would outgrow its maximum size: a limit, not the system running out of memory
        code = ErrorCode::ResourceLimitExceeded;
        message = ex.what();
    }
    catch (const std::exception& ex)
    {
        message = ex.what();
    }
    catch (...) // NOLINT(bugprone-empty-catch): the defaults above describe it
    {
    }
    // Still in the caller's handler: the exception, and with it `message`, is alive
    typename BasicErrorInfo<CharT>::Data errorData;
    errorData.code = code;
    errorData.srcLoc.fileName = fileName;
    errorData.srcLoc.line = 1;
    errorData.srcLoc.col = 1;
    errorData.extraParams.emplace_back(std::string(message));
    if (limitField)
    {
        errorData.extraParams.emplace_back(std::string(limitField));
    }
    if (fromOutside)
    {
        errorData.exception = std::current_exception();
    }
    return BasicErrorInfo<CharT>(std::move(errorData));
}
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_ERROR_BOUNDARY_H

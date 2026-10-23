#ifndef JINJA2CPP_SRC_ERROR_BOUNDARY_H
#define JINJA2CPP_SRC_ERROR_BOUNDARY_H

#include "engine_errors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/string_helpers.h>

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
// The error a thrown ErrorInfo describes, as a template of CharT reports it
template<typename CharT, typename SrcCharT>
typename BasicErrorInfo<CharT>::Data CopyErrorData(const BasicErrorInfo<SrcCharT>& error)
{
    typename BasicErrorInfo<CharT>::Data errorData;
    errorData.code = error.GetCode();
    errorData.srcLoc = error.GetErrorLocation();
    errorData.relatedLocs = error.GetRelatedLocations();
    errorData.extraParams = error.GetExtraParams();
    errorData.locationDescr = ConvertString<std::basic_string<CharT>>(error.GetLocationDescr());
    errorData.exception = error.GetException();
    return errorData;
}

// The error for the exception being handled (docs/api-2.0.md, 2.1): its code, its what() and,
// when the engine did not raise it, the exception itself. A thrown ErrorInfo stays as it is;
// `located` tells whether it has a location of its own, which only a parse error has (the
// location, else, is for the caller to set). Call it from a catch handler only. std::bad_alloc
// goes on unchanged, before anything allocates.
template<typename CharT>
typename BasicErrorInfo<CharT>::Data CurrentExceptionToData(bool& located)
{
    located = false;
    ErrorCode code = ErrorCode::UnexpectedException;
    const char* message = "unknown exception";
    // Kept only for an exception raised outside the engine (docs/api-2.0.md, 2.1)
    std::exception_ptr exception = std::current_exception();
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
    catch (const BasicErrorInfo<char>& error)
    {
        located = !error.GetLocationDescr().empty();
        return CopyErrorData<CharT>(error);
    }
    catch (const BasicErrorInfo<wchar_t>& error)
    {
        located = !error.GetLocationDescr().empty();
        return CopyErrorData<CharT>(error);
    }
    catch (const HostCallableFailure& failure)
    {
        code = ErrorCode::UserCallableException;
        exception = failure.Exception();
        try
        {
            std::rethrow_exception(exception);
        }
        catch (const std::exception& ex)
        {
            // `exception` keeps it, and with it the message, alive
            message = ex.what();
        }
        catch (...) // NOLINT(bugprone-empty-catch): the default message describes it
        {
        }
    }
    catch (const Exception& ex)
    {
        code = ex.Code();
        message = ex.what();
    }
    catch (const UndefinedError& ex)
    {
        code = ErrorCode::UndefinedError;
        message = ex.what();
        exception = nullptr;
    }
    catch (const TemplateRuntimeError& ex)
    {
        code = ErrorCode::TemplateRuntimeError;
        message = ex.what();
        exception = nullptr;
    }
    catch (const RecursionLimitError&)
    {
        code = ErrorCode::RecursionLimitExceeded;
        message = nullptr;
        exception = nullptr;
    }
    catch (const ResourceLimitError& ex)
    {
        code = ErrorCode::ResourceLimitExceeded;
        message = ex.what();
        limitField = ex.Field();
        exception = nullptr;
    }
    catch (const IntegerOutOfRangeError& ex)
    {
        code = ErrorCode::IntegerOutOfRange;
        message = ex.what();
        exception = nullptr;
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
    if (message)
    {
        errorData.extraParams.emplace_back(std::string(message));
    }
    if (limitField)
    {
        errorData.extraParams.emplace_back(std::string(limitField));
    }
    errorData.exception = std::move(exception);
    return errorData;
}

// The error a public Load or Render boundary returns for the exception being handled, at the
// start of `fileName` unless it has a location of its own (CurrentExceptionToData)
template<typename CharT>
BasicErrorInfo<CharT> CurrentExceptionToError(const std::string& fileName)
{
    bool located = false;
    auto errorData = CurrentExceptionToData<CharT>(located);
    if (!located)
    {
        errorData.srcLoc.fileName = fileName;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.col = 1;
    }
    return BasicErrorInfo<CharT>(std::move(errorData));
}
// The error a public Load boundary returns for what host code (the autoescape selector) threw:
// as CurrentExceptionToError, but a thrown ErrorInfo is the host's too, with no code of the
// engine's to report
template<typename CharT>
BasicErrorInfo<CharT> HostExceptionToError(const std::string& fileName)
{
    try
    {
        throw;
    }
    catch (const BasicErrorInfo<char>&) // NOLINT(bugprone-empty-catch): described below
    {
    }
    catch (const BasicErrorInfo<wchar_t>&) // NOLINT(bugprone-empty-catch): described below
    {
    }
    catch (...)
    {
        return CurrentExceptionToError<CharT>(fileName);
    }
    typename BasicErrorInfo<CharT>::Data errorData;
    errorData.code = ErrorCode::UnexpectedException;
    errorData.srcLoc.fileName = fileName;
    errorData.srcLoc.line = 1;
    errorData.srcLoc.col = 1;
    errorData.extraParams.emplace_back(std::string("unknown exception"));
    errorData.exception = std::current_exception();
    return BasicErrorInfo<CharT>(std::move(errorData));
}
} // namespace detail
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_ERROR_BOUNDARY_H

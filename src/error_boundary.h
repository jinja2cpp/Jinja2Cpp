#ifndef JINJA2CPP_SRC_ERROR_BOUNDARY_H
#define JINJA2CPP_SRC_ERROR_BOUNDARY_H

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>

#include <exception>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>

namespace jinja2::detail
{
// The error a public Load or Render boundary returns for the exception being handled, one the
// engine did not raise (docs/api-2.0.md, 2.1): its code, its what() and the exception itself.
// Call it from a catch handler only. std::bad_alloc goes on unchanged, before anything allocates.
template<typename CharT>
BasicErrorInfo<CharT> CurrentExceptionToError(const std::string& fileName)
{
    auto make = [&fileName](ErrorCode code, const char* message) {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = code;
        errorData.srcLoc.fileName = fileName;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.col = 1;
        errorData.extraParams.emplace_back(std::string(message));
        errorData.exception = std::current_exception();
        return BasicErrorInfo<CharT>(std::move(errorData));
    };
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
        return make(ex.Code(), ex.what());
    }
    catch (const std::length_error& ex)
    {
        // A string or container would outgrow its maximum size: a limit, not the system running out of memory
        return make(ErrorCode::ResourceLimitExceeded, ex.what());
    }
    catch (const std::exception& ex)
    {
        return make(ErrorCode::UnexpectedException, ex.what());
    }
    catch (...)
    {
        return make(ErrorCode::UnexpectedException, "unknown exception");
    }
}
} // namespace jinja2::detail

#endif // JINJA2CPP_SRC_ERROR_BOUNDARY_H

#ifndef JINJA2CPP_PARSE_RESULT_H
#define JINJA2CPP_PARSE_RESULT_H

#include "error_info.h"

#include <boost/optional.hpp>

#include <iostream>

namespace jinja2
{
template<typename CharT>
class ParseResultTpl
{
public:
    using ThisType = ParseResultTpl<CharT>;
    using ErrorType = ErrorInfoTpl<CharT>;

    ParseResultTpl() = default;
    explicit ParseResultTpl(ErrorType&& error)
        : m_error(std::move(error))
    {
    }

    bool operator!() const
    {
        return m_error;
    }
    operator bool() const
    {
        return !m_error;
    }

    bool HasError() const
    {
        return static_cast<bool>(m_error);
    }

    auto GetError() const
    {
        return m_error.get_ptr();
    }

private:
    boost::optional<ErrorType> m_error;

};

using ParseResult = ParseResultTpl<char>;
using ParseResultW = ParseResultTpl<wchar_t>;

inline std::ostream& operator << (std::ostream& os, const ParseResult& res)
{
    if (res)
        os << "OK";
    else
        os << *res.GetError();
    return os;
}

inline std::wostream& operator << (std::wostream& os, const ParseResultW& res)
{
    if (res)
        os << L"OK";
    else
        os << *res.GetError();
    return os;
}

} // jinja2

#endif // JINJA2CPP_PARSE_RESULT_H

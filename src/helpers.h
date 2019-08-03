#ifndef HELPERS_H
#define HELPERS_H

#include <nonstd/string_view.hpp>

#include <string>
#include <type_traits>
#include <cwchar>

namespace jinja2
{
struct MultiStringLiteral
{
    const char* charValue;
    const wchar_t* wcharValue;

    template<typename CharT>
    auto GetValue() const
    {
        auto memPtr = SelectMemberPtr<CharT, &MultiStringLiteral::charValue, &MultiStringLiteral::wcharValue>::GetPtr();
        return std::basic_string<CharT>(this->*memPtr);
    }

    template<typename CharT, const char* MultiStringLiteral::*, const wchar_t* MultiStringLiteral::*>
    struct SelectMemberPtr;

    template<const char* (MultiStringLiteral::*charMemPtr), const wchar_t* (MultiStringLiteral::*wcharMemPtr)>
    struct SelectMemberPtr<char, charMemPtr, wcharMemPtr>
    {
        static auto GetPtr() {return charMemPtr;}
    };

    template<const char* (MultiStringLiteral::*charMemPtr), const wchar_t* (MultiStringLiteral::*wcharMemPtr)>
    struct SelectMemberPtr<wchar_t, charMemPtr, wcharMemPtr>
    {
        static auto GetPtr() {return wcharMemPtr;}
    };

    template<typename CharT>
    friend std::basic_ostream<CharT>& operator << (std::basic_ostream<CharT>& os, const MultiStringLiteral& lit)
    {
        os << lit.GetValue<CharT>();
        return os;
    }
};

#define UNIVERSAL_STR(Str) MultiStringLiteral{Str, L##Str}

namespace detail
{
template<typename Src, typename Dst>
struct StringConverter;

template<typename Src>
struct StringConverter<Src, Src>
{
    static Src DoConvert(const nonstd::basic_string_view<typename Src::value_type>& from)
    {
        return Src(from.begin(), from.end());
    }
};

template<>
struct StringConverter<std::wstring, std::string>
{
    static std::string DoConvert(const nonstd::wstring_view& from)
    {
        std::mbstate_t state = std::mbstate_t();
        auto src = from.data();
#ifndef _MSC_VER
        std::size_t len = 1 + std::wcsrtombs(nullptr, &src, 0, &state);
#else
        std::size_t len = 0;
        auto err = wcsrtombs_s(&len, nullptr, 0, &src, 0, &state);
        if (err != 0)
            return std::string();
        ++ len;
#endif
        std::string result;
        result.resize(len);
        src = from.data();
#ifndef _MSC_VER
        std::wcsrtombs(&result[0], &src, from.size(), &state);
#else
        wcsrtombs_s(&len, &result[0], len, &src, from.size(), &state);
#endif
        return result;
    }
};

template<>
struct StringConverter<std::string, std::wstring>
{
    static std::wstring DoConvert(const nonstd::string_view& from)
    {
        std::mbstate_t state = std::mbstate_t();
        auto src = from.data();
#ifndef _MSC_VER
        std::size_t len = 1 + std::mbsrtowcs(NULL, &src, 0, &state);
#else
        std::size_t len = 0;
        auto err = mbsrtowcs_s(&len, NULL, 0, &src, 0, &state);
        if (err != 0)
            return std::wstring();
        ++len;
#endif
        std::wstring result;
        result.resize(len);
        src = from.data();
#ifndef _MSC_VER
        std::mbsrtowcs(&result[0], &src, result.size(), &state);
#else
        mbsrtowcs_s(&len, &result[0], len, &src, result.size(), &state);
#endif
        return result;
    }
};

template<typename CharT, typename T>
struct StringConverter<nonstd::basic_string_view<CharT>, T> : public StringConverter<std::basic_string<CharT>, T> {};

} // detail

template<typename Dst, typename Src>
Dst ConvertString(Src&& from)
{
    using src_t = std::decay_t<Src>;
    return detail::StringConverter<src_t, std::decay_t<Dst>>::DoConvert(nonstd::basic_string_view<typename src_t::value_type>(from));
}

//! CompileEscapes replaces escape characters by their meanings.
/**
 * @param[in] s Characters sequence with zero or more escape characters.
 * @return Characters sequence copy where replaced all escape characters by
 * their meanings.
 */
template<typename Sequence>
Sequence CompileEscapes(Sequence s)
{
   auto itr1 = s.begin();
   auto itr2 = s.begin();
   const auto end = s.cend();

   auto removalCount  = 0;

   while (end != itr1)
   {
      if ('\\' == *itr1)
      {
         ++removalCount;

         if (end == ++itr1)
            break;
         if ('\\' != *itr1)
         {
            switch (*itr1)
            {
               case 'n': *itr1 = '\n'; break;
               case 'r': *itr1 = '\r'; break;
               case 't': *itr1 = '\t'; break;
               default:                break;
            }

            continue;
         }
      }

      if (itr1 != itr2)
         *itr2 = *itr1;

      ++itr1;
      ++itr2;
   }

   s.resize(s.size() - removalCount);

   return s;
}

} // jinja2

#endif // HELPERS_H

#ifndef HELPERS_H
#define HELPERS_H

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
    static Src DoConvert(const Src& from)
    {
        return from;
    }
};

template<>
struct StringConverter<std::wstring, std::string>
{
    static std::string DoConvert(const std::wstring& from)
    {
        std::mbstate_t state = std::mbstate_t();
        auto srcPtr = from.data();
        std::size_t srcSize = from.size();
        std::size_t destBytes = 0;

#ifndef _MSC_VER
        destBytes = std::wcsrtombs(nullptr, &srcPtr, srcSize, &state);
        if (destBytes == (size_t)-1)
            return std::string();
#else
        auto err = wcsrtombs_s(&destBytes, nullptr, 0, &srcPtr, srcSize, &state);
        if (err != 0)
            return std::string();
#endif
        std::string result;
        result.resize(destBytes);
#ifndef _MSC_VER
        std::wcsrtombs(&result[0], &srcPtr, srcSize, &state);
#else
        wcsrtombs_s(&destBytes, &result[0], destBytes, &srcPtr, srcSize, &state);
        result.resize(destBytes-1);
#endif
        return result;
    }
};

template<>
struct StringConverter<std::string, std::wstring>
{
    static std::wstring DoConvert(const std::string& from)
    {
        std::mbstate_t state = std::mbstate_t();
        auto srcPtr = from.data();
        std::size_t srcSize = from.size();
        std::size_t destBytes = 0;

#ifndef _MSC_VER
        destBytes = std::mbsrtowcs(nullptr, &srcPtr, srcSize, &state);
        if (destBytes == (size_t)-1)
            return std::wstring();
#else
        auto err = mbsrtowcs_s(&destBytes, nullptr, 0, &srcPtr, srcSize, &state);
        if (err != 0)
            return std::wstring();
#endif
        std::wstring result;
        result.resize(destBytes);
#ifndef _MSC_VER
        std::mbsrtowcs(&result[0], &srcPtr, srcSize, &state);
#else
        mbsrtowcs_s(&destBytes, &result[0], destBytes, &srcPtr, srcSize, &state);
        result.resize(destBytes-1);
#endif
        return result;
    }
};

} // detail

template<typename Dst, typename Src>
Dst ConvertString(Src&& from)
{
    return detail::StringConverter<std::decay_t<Src>, std::decay_t<Dst>>::DoConvert(std::forward<Src>(from));
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

// Copyright Kevlin Henney, 2000-2005.
// Copyright Alexander Nasonov, 2006-2010.
// Copyright Antony Polukhin, 2011-2014.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// what:  lexical_cast custom keyword cast
// who:   contributed by Kevlin Henney,
//        enhanced with contributions from Terje Slettebo,
//        with additional fixes and suggestions from Gennaro Prota,
//        Beman Dawes, Dave Abrahams, Daryle Walker, Peter Dimov,
//        Alexander Nasonov, Antony Polukhin, Justin Viiret, Michael Hofmann,
//        Cheng Yang, Matthew Bradbury, David W. Birdsall, Pavel Korzh and other Boosters
// when:  November 2000, March 2003, June 2005, June 2006, March 2011 - 2014

#ifndef _LEXICAL_CAST_H_
#define _LEXICAL_CAST_H_

#include <climits>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <cstring>
#include <cstdio>
#include <sstream>
#include "mpl.h"

namespace detail {

// selectors for choosing stream character type
template<typename Type>
struct stream_char
{
    typedef char type;
};

template<>
struct stream_char<wchar_t*>
{
    typedef wchar_t type;
};

template<>
struct stream_char<const wchar_t*>
{
    typedef wchar_t type;
};

template<>
struct stream_char<std::wstring>
{
    typedef wchar_t type;
};

template <typename T1, typename T2>
struct widest_char
{
    typedef typename mpl::if_ <
    (sizeof(T1) > sizeof(T2)),
    T1,
    T2
    >::type type;
};

// stream wrapper for handling lexical conversions
template<typename Target, typename Source, typename Traits>
class lexical_stream
{
private:
    typedef typename widest_char <
    typename stream_char<Target>::type,
             typename stream_char<Source>::type >::type char_type;

    typedef Traits traits_type;

public:
    lexical_stream(char_type* = 0, char_type* = 0)
    {
        stream.unsetf(std::ios::skipws);
        lcast_set_precision(stream, static_cast<Source*>(0), static_cast<Target*>(0) );
    }
    ~lexical_stream()
    { }

    bool operator<<(const Source& input)
    {
        return !(stream << input).fail();
    }
    template<typename InputStreamable>
    bool operator>>(InputStreamable& output)
    {
        return !std::is_pointer<InputStreamable>::value &&
               stream >> output &&
               stream.get() == traits_type::eof();
    }

    bool operator>>(std::string& output)
    {
        stream.str().swap(output);
        return true;
    }
    //bool operator>>(std::wstring &output)
    //{
    //	stream.str().swap(output);
    //	return true;
    //}

private:
    std::stringstream stream;
};
} // namespace detail

// call-by-value fallback version (deprecated)

template <typename Target, typename Source>
Target lexical_cast(Source arg)
{
    typedef typename detail::widest_char <
    typename detail::stream_char<Target>::type
    , typename detail::stream_char<Source>::type
    >::type char_type;

    typedef std::char_traits<char_type> traits;
    detail::lexical_stream<Target, Source, traits> interpreter;
    Target result;

    if (!(interpreter << arg && interpreter >> result))
        throw std::invalid_argument("bad lexical_cast");
    return result;
}

#endif // _LEXICAL_CAST_H_



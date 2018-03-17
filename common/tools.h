
#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "integer_cast.h"
#include "range.h"

#ifdef _MSC_VER
	#define _HAVE_CXX11_ 1
	#if _MSC_VER == 1700
		#define NO_TR1 1
		#define snprintf(b, c, format, ...) _snprintf_s(b, c, c, format, ##__VA_ARGS__)
	#endif
	#ifndef bzero
		#define bzero(p, s)    memset(p, 0, s)
		#define bcopy(s, p, n) memcpy(p, s, n)
	#endif // bzero
#endif // _MSC_VER

//#if defined(_HAVE_CXX11_) && !defined(NO_TR1)
#include "prettyprint.h"
//#else
//#include "prettyprint98.h"
//#endif

namespace detail
{
    inline int length_of(char)
    {
        return 1;
    }

    template <size_t N>
    inline int length_of(const char (&x)[N])
    {
        return integer_cast<int>(N-1);
    }

    inline int length_of(const std::string& x)
    {
        return integer_cast<int>(x.length());
    }

    template <int N>
    struct array_size_struct
    {
        uint8_t c[N];
    };

    template <class T, int N>
    array_size_struct<N> static_array_size_fn(T (&)[N]);
} // namespace detail

#define dimensionof(x) sizeof(detail::static_array_size_fn(x).c)
#define dimof(x) sizeof(detail::static_array_size_fn(x).c)

#define ISDIGIT(X)   (((X) >= '0') && ((X) <= '9'))
#define ISODIGIT(X)  (((X) >= '0') && ((X) <= '7'))
#define VALID_HEX(X) ((((X) >= '0') && ((X) <= '9')) || (((X) >= 'a') && ((X) <= 'f')) || (((X) >= 'A') && ((X) <= 'F')))
#define ISXDIGIT(X)  (VALID_HEX(X))

template <size_t N>
inline StringPiece make_stringpiece(const char (&str)[N])
{
    return StringPiece(str);
}

inline StringPiece make_stringpiece(const std::string& str)
{
    return StringPiece(str.data(), detail::integer_cast<int>(str.size()));
}

inline StringPiece make_stringpiece(const void* data, int len)
{
    return StringPiece(reinterpret_cast<const char*>(data), len);
}

inline StringPiece make_stringpiece(const void* beg, const void* end)
{
    return StringPiece(reinterpret_cast<const char*>(beg), reinterpret_cast<const char*>(end));
}

#endif // _TOOLS_H_


#ifndef _DEFINE_H_
#define _DEFINE_H_

#ifdef _MSC_VER
#define _HAVE_CXX11_ 1
#  if _MSC_VER == 1700
#    define NO_TR1 1
#    define snprintf(b, c, format, ...) _snprintf_s(b, c, c, format, ##__VA_ARGS__)
#  endif
#define bzero(p, s)    memset(p, 0, s)
#define bcopy(s, p, n) memcpy(p, s, n)
#endif

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <iostream>
#include <string>
#include <sstream>
#include <type_traits>

#ifndef _HAVE_CXX11_
#  ifndef nullptr
#    define nullptr      (0)
#  endif // nullptr
#endif // _HAVE_CXX11_

// macros for cstring
#define CONST_LENGTH (512)

// macros for qmatch
#define MAX_VERSION (std::numeric_limits<uint64_t>::max() / 2)

typedef uintptr_t TNID;

enum DecodeType
{
    kNone,
    kUrlDecodeUni,
    kHtmlEntityDecode
};

//  IDE's like Visual Studio perform better if output goes to std::cout or
//  some other stream, so allow user to configure output stream:
#ifndef LOG_OSTREAM
    #define LOG_OSTREAM std::clog
#endif

#ifndef ASSERT_OSTREAM
    #define ASSERT_OSTREAM std::cerr
#endif

#ifdef NDEBUG_LOG
    #define NEW_BLOCK_LOG(prefix, text)
    #define LOG_APPEND(text)
    #define LOG_APPEND2(text, suffix)
    #define LOG_APPEND3(prefix, text, suffix)
#else
    #define NEW_BLOCK_LOG(prefix, text) { \
        LOG_OSTREAM << (prefix) << " \"" << (text) << "\"" << " @" << __LINE__ << std::endl; \
    } while (0)

    #define LOG_APPEND(text) do { \
        LOG_OSTREAM << (text); \
    } while (0)

    #define LOG_APPEND2(text, suffix) do { \
        LOG_OSTREAM << (text) << (suffix); \
    } while (0)

    #define LOG_APPEND3(prefix, text, suffix) do { \
        LOG_OSTREAM << (prefix) << (text) << (suffix); \
    } while (0)
#endif // NDEBUG

template <class T>
inline bool is_nullptr(const T* p)
{
    return (p == nullptr);
}

// macros for trie tree
#define TK_INVAILD static_cast<uintptr_t>(-1)
//#define MINIMUM_LETTER_SET
#define MIN_LETTER (0)
#define MAX_LETTER (255)
//#define TRIE_DEBUG

// macros for regexpr
#define SPECIAL_CHAR     0x0C
#ifndef REGEX_MAX_NODES
    #define REGEX_MAX_NODES  307
#endif // REGEX_MAX_NODES

#define ISDIGIT(X)   (((X) >= '0') && ((X) <= '9'))
#define ISODIGIT(X)  (((X) >= '0') && ((X) <= '7'))
#define VALID_HEX(X) ((((X) >= '0') && ((X) <= '9')) || (((X) >= 'a') && ((X) <= 'f')) || (((X) >= 'A') && ((X) <= 'F')))
#define ISXDIGIT(X)  (VALID_HEX(X))

#endif // _DEFINE_H_

#ifndef _LIGHTWEIGHT_TEST_HPP_
#define _LIGHTWEIGHT_TEST_HPP_

// MS compatible compilers support #pragma once

#if defined(_MSC_VER)
# pragma once
#endif

//
//  boost/core/lightweight_test.hpp - lightweight test library
//
//  Copyright (c) 2002, 2009, 2014 Peter Dimov
//  Copyright (2) Beman Dawes 2010, 2011
//  Copyright (3) Ion Gaztanaga 2013
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
//

#include "common/smart_assert.h"

#include <cassert>
#include <iostream>

#define BOOST_ASSERT SMART_ASSERT

//  IDE's like Visual Studio perform better if output goes to std::cout or
//  some other stream, so allow user to configure output stream:
#ifndef LIGHTWEIGHT_TEST_OSTREAM
# define LIGHTWEIGHT_TEST_OSTREAM std::cerr
#endif

namespace detail
{

inline void current_function_helper()
{

#if defined(__GNUC__) || (defined(__MWERKS__) && (__MWERKS__ >= 0x3000)) || (defined(__ICC) && (__ICC >= 600)) || defined(__ghs__)

# define BOOST_CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__DMC__) && (__DMC__ >= 0x810)

# define BOOST_CURRENT_FUNCTION __PRETTY_FUNCTION__

#elif defined(__FUNCSIG__)

# define BOOST_CURRENT_FUNCTION __FUNCSIG__

#elif (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 600)) || (defined(__IBMCPP__) && (__IBMCPP__ >= 500))

# define BOOST_CURRENT_FUNCTION __FUNCTION__

#elif defined(__BORLANDC__) && (__BORLANDC__ >= 0x550)

# define BOOST_CURRENT_FUNCTION __FUNC__

#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)

# define BOOST_CURRENT_FUNCTION __func__

#elif defined(__cplusplus) && (__cplusplus >= 201103)

# define BOOST_CURRENT_FUNCTION __func__

#else

# define BOOST_CURRENT_FUNCTION "(unknown)"

#endif

}

struct report_errors_reminder
{
    bool called_report_errors_function;

    report_errors_reminder() : called_report_errors_function(false) {}

    ~report_errors_reminder()
    {
        BOOST_ASSERT(called_report_errors_function);  // verify report_errors() was called  
    }
};

inline report_errors_reminder& report_errors_remind()
{
    static report_errors_reminder r;
    return r;
}

inline int & test_errors()
{
    static int x = 0;
    report_errors_remind();
    return x;
}

inline void test_failed_impl(char const * expr, char const * file, int line, char const * function)
{
    LIGHTWEIGHT_TEST_OSTREAM
      << file << "(" << line << "): test '" << expr << "' failed in function '"
      << function << "'" << std::endl;
    ++test_errors();
}

inline void error_impl(char const * msg, char const * file, int line, char const * function)
{
    LIGHTWEIGHT_TEST_OSTREAM
      << file << "(" << line << "): " << msg << " in function '"
      << function << "'" << std::endl;
    ++test_errors();
}

inline void throw_failed_impl(char const * excep, char const * file, int line, char const * function)
{
   LIGHTWEIGHT_TEST_OSTREAM
    << file << "(" << line << "): Exception '" << excep << "' not thrown in function '"
    << function << "'" << std::endl;
   ++test_errors();
}

template<class T, class U> inline void test_eq_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const & t, U const & u )
{
    if( t == u )
    {
        report_errors_remind();
    }
    else
    {
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " == " << expr2
            << "' failed in function '" << function << "': "
            << "'" << t << "' != '" << u << "'" << std::endl;
        ++test_errors();
    }
}

template<class T, class U> inline void test_ne_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const & t, U const & u )
{
    if( t != u )
    {
        report_errors_remind();
    }
    else
    {
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " != " << expr2
            << "' failed in function '" << function << "': "
            << "'" << t << "' == '" << u << "'" << std::endl;
        ++test_errors();
    }
}

template<class T> inline void test_memory_eq_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const * t, const size_t& tn, T const * u)
{  
    if( memcmp(t, u, tn) == 0 )
    {   
        report_errors_remind();
    }  
    else
    {   
        BOOST_ASSERT(0);
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " == " << expr2
            << "' failed in function '" << function << "': "
            << "'" << (uintptr_t)t << "' != '" << (uintptr_t)u << "'" << std::endl;
        ++test_errors();
    }
}

template<class T> inline void test_memory_ne_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const * t, const size_t& tn, T const * u)
{  
    if( memcmp(t, u, tn) != 0 )
    {   
        report_errors_remind();
    }  
    else
    {
        BOOST_ASSERT(0);
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " != " << expr2
            << "' failed in function '" << function << "': "
            << "'" << (uintptr_t)t << "' == '" << (uintptr_t)u << "'" << std::endl;
        ++test_errors();
    }
}

template<class T, size_t N1, size_t N2> inline void test_array_eq_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const (&t)[N1], T const (&u)[N2])
{   
    if( (N1 == N2) && memcmp(t, u, sizeof(T) * N1) == 0 )
    {   
        report_errors_remind();
    }
    else
    {
        BOOST_ASSERT(0);
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " == " << expr2
            << "' failed in function '" << function << "': "
            << "'" << t << "' != '" << u << "'" << std::endl;
        ++test_errors();
    }
}

template<class T, size_t N1, size_t N2> inline void test_array_ne_impl( char const * expr1, char const * expr2,
  char const * file, int line, char const * function, T const (&t)[N1], T const (&u)[N2])
{   
    if( (N1 != N2) || memcmp(t, u, sizeof(T) * N1) != 0 )
    {   
        report_errors_remind();
    }
    else
    {
        BOOST_ASSERT(0);
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): test '" << expr1 << " != " << expr2
            << "' failed in function '" << function << "': "
            << "'" << t << "' == '" << u << "'" << std::endl;
        ++test_errors();
    }
}

template< class T > inline void test_trait_impl( char const * trait, void (*)( T ),
  bool expected, char const * file, int line, char const * function )
{
    if( T::value == expected )
    {   
        report_errors_remind();
    }
    else
    {   
        LIGHTWEIGHT_TEST_OSTREAM
            << file << "(" << line << "): predicate '" << trait
            << " test failed in function '" << function
            << "' (should have been " << ( expected? "true": "false" ) << ")"
            << std::endl;

        ++test_errors();
    }
}

inline int length_of(const char* x)
{
    return integer_cast<int>(strlen(x));
}

} // namespace detail

inline int report_errors()
{
    detail::report_errors_remind().called_report_errors_function = true;

    int errors = detail::test_errors();

    if( errors == 0 )
    {
        LIGHTWEIGHT_TEST_OSTREAM
          << "No errors detected." << std::endl;
        return 0;
    }
    else
    {
        LIGHTWEIGHT_TEST_OSTREAM
          << errors << " error" << (errors == 1? "": "s") << " detected." << std::endl;
        return 1;
    }
}

#define BOOST_TEST(expr) ((expr)? (void)0: detail::test_failed_impl(#expr, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION))

#define BOOST_ERROR(msg) ( detail::error_impl(msg, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION) )

#define BOOST_TEST_EQ(expr1,expr2) ( detail::test_eq_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, expr2) )
#define BOOST_TEST_NE(expr1,expr2) ( detail::test_ne_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, expr2) )

#define BOOST_TEST_MEMORY_EQ(expr1,expr2,bytes) (detail::test_memory_eq_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, bytes, expr2)
#define BOOST_TEST_MEMORY_NE(expr1,expr2,bytes) (detail::test_memory_ne_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, bytes, expr2)

#define BOOST_TEST_ARRAY_EQ(expr1,expr2) (detail::test_array_eq_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, expr2))
#define BOOST_TEST_ARRAY_NE(expr1,expr2) (detail::test_array_ne_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, expr2))

#define BOOST_TEST_STREQ(expr1,expr2) (detail::test_memory_eq_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, detail::length_of(expr1), expr2))
#define BOOST_TEST_STRNE(expr1,expr2) (detail::test_memory_ne_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, expr1, detail::length_of(expr1), expr2))

#define BOOST_TEST_STROBJ_EQ(expr1,expr2) (detail::test_eq_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, make_stringpiece(expr1), make_stringpiece(expr2)))
#define BOOST_TEST_STROBJ_NE(expr1,expr2) (detail::test_ne_impl(#expr1, #expr2, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION, make_stringpiece(expr1), make_stringpiece(expr2)))

#define BOOST_TEST_TRAIT_TRUE(type) ( detail::test_trait_impl(#type, (void(*)type)0, true, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION) )
#define BOOST_TEST_TRAIT_FALSE(type) ( detail::test_trait_impl(#type, (void(*)type)0, false, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION) )

#define BOOST_TEST_THROWS( EXPR, EXCEP )                    \
   try {                                                    \
      EXPR;                                                 \
      detail::throw_failed_impl                    \
      (#EXCEP, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION); \
   }                                                        \
   catch(EXCEP const&) {                                    \
   }                                                        \
   catch(...) {                                             \
      detail::throw_failed_impl                    \
      (#EXCEP, __FILE__, __LINE__, BOOST_CURRENT_FUNCTION); \
   }                                                        \
//

#endif // #ifndef _LIGHTWEIGHT_TEST_HPP_


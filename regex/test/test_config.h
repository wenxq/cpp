
#ifndef _TEST_TIME_
#define _TEST_TIME_

#include "common/slice.h"
#include "common/xstring.h"
#include "lightweight_test.hpp"
#include <string.h>
#include <time.h>

typedef XString<1024> CString;

#define TIME_TYPE clock_t
#define GET_TIME(res) do { res = clock(); } while(0)
#define GET_TIME_RESOLUTION(res) do { res = CLOCKS_PER_SEC; } while(0)
#define TIME_DIFF_IN_MS(begin, end, resolution) (TIME_TYPE)(((double)((end) - (begin)) * 1000 / (resolution)))

#define DURATION(msg, t1, t2) do { \
    TIME_TYPE resolution; \
    GET_TIME_RESOLUTION(resolution); \
    const TIME_TYPE xdiff = TIME_DIFF_IN_MS(t1, t2, resolution); \
    BOOST_AUTO(w, std::cout.width()); \
    std::cout.width(30); \
    std::cout << #msg << ": " << xdiff << std::endl; \
    std::cout.width(w); \
} while (0)

#define ASSERT_TRUE(a)         BOOST_TEST(a)
#define ASSERT_FALSE(a)        BOOST_TEST(!a)
#define ASSERT_EQ(a, b)        BOOST_TEST_EQ(a, b)
#define ASSERT_NE(a, b)        BOOST_TEST_NE(a, b)
#define ASSERT_STREQ(a, b)     BOOST_TEST_STREQ(a, b)
#define ASSERT_STROBJ_EQ(a, b) BOOST_TEST_STROBJ_EQ(a, b)

#endif // _TEST_TIME_

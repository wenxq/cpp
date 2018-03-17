
#ifndef _SMART_ASSERT_H_
#define _SMART_ASSERT_H_

#include "preprocessor.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>

#ifndef ASSERT_ACTION
    #ifdef _MSC_VER
        #include <crtdbg.h>
        #define ASSERT_ACTION(str) do { \
                if (1 == _CrtDbgReport(_CRT_ASSERT, "", 0, NULL, str.c_str())) { \
                    _CrtDbgBreak(); \
                } \
            } while(0)
    #else
        #define ASSERT_ACTION(str) abort()
    #endif
#endif // ASSERT_ACTION

#if defined(NO_STACK_TRACE)
    #define FILL_STACK_TRACE(os)
#else
    #define FILL_STACK_TRACE(os) fill_stack_trace(os)
    void fill_stack_trace(std::ostream& os);
    inline void fill_stack_trace(std::string& st)
    {
        std::ostringstream ss;
        fill_stack_trace(ss);
        st.assign(ss.str());
    }
#endif // NO_STACK_TRACE

#ifdef NDEBUG
    struct EmptyAssert
    {
        template <class T>
        const EmptyAssert& operator()(const char* name, const T& obj) const
        {
            return *this;
        }

        const EmptyAssert& msg(const char* message) const
        {
            return *this;
        }
    };

    #define SMART_ASSERT(expr) \
        if (1) ; \
        else EmptyAssert()
#else

#ifndef ASSERT_OSTREAM
#define ASSERT_OSTREAM std::cerr
#endif // ASSERT_OSTREAM

    class SmartAssert
    {
    public:
        SmartAssert(const char* expr, const char* file, int lineno, const char* func)
          : mValues(false)
        {
            mStream << "Assertion failed in " << ((func == nullptr) ? "null" : func) << '@' << file << ':' << lineno
                    << ":\nExpression: \'" << expr << "\'\n";
        }

        ~SmartAssert()
        {
            ASSERT_OSTREAM << mStream.str() << std::flush;
            FILL_STACK_TRACE(ASSERT_OSTREAM);
            ASSERT_ACTION(mStream.str());
        }

        template <class K, class T>
        SmartAssert& operator()(const K& name, const T& obj)
        {
            if (!mValues)
            {
                mStream << "Values: \n";
                mValues = true;
            }

            mStream << name << " = " << obj << "\n";

            return *this;
        }

        SmartAssert& msg(const char* message)
        {
            mStream << "msg: " << message << "\n";
            return *this;
        }

    private:
        SmartAssert(const SmartAssert&);
        SmartAssert& operator=(const SmartAssert&);

    private:
        std::ostringstream mStream;
        bool mValues;
    };

    #define SMART_ASSERT(expr) \
        if ((expr)) ; \
        else SmartAssert(#expr, __FILE__, __LINE__, CURRENT_FUNCTION)
#endif // NDEBUG

#endif // _SMART_ASSERT_H_

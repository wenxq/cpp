
#ifndef _PREPROCESSOR_H_
#define _PREPROCESSOR_H_

#undef HAVE_INT128_T

#ifndef _MSC_VER
    #ifndef nullptr
        #define nullptr 0
    #endif
#endif

#undef CURRENT_FUNCTION
#ifndef _MSC_VER
    #define CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
    #if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 199901L
        #define CURRENT_FUNCTION __func__
    #else
        #define CURRENT_FUNCTION __FUNCTION__
    #endif // __STDC_VERSION__
#endif //_MSC_VER

#if defined(__has_builtin)
	#define HAS_BUILTIN(...) __has_builtin(__VA_ARGS__)
#else
	#define HAS_BUILTIN(...) 0
#endif // __has_builtin

#if defined(__has_feature)
	#define HAS_FEATURE(...) __has_feature(__VA_ARGS__)
#else
	#define HAS_FEATURE(...) 0
#endif // __has_feature

// deprecated
#if defined(__clang__) || defined(__GNUC__)
	#define DEPRECATED(msg) __attribute__((__deprecated__(msg)))
#elif defined(_MSC_VER)
	#define DEPRECATED(msg) __declspec(deprecated(msg))
#else
	#define DEPRECATED(msg)
#endif

// noinline
#ifdef _MSC_VER
	#define NOINLINE __declspec(noinline)
#elif defined(__clang__) || defined(__GNUC__)
	#define NOINLINE __attribute__((__noinline__))
#else
	#define NOINLINE
#endif

// always inline
#ifdef _MSC_VER
	#define ALWAYS_INLINE __forceinline
#elif defined(__clang__) || defined(__GNUC__)
	#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
	#define ALWAYS_INLINE inline
#endif

#undef BOOST_AUTO
#if defined(__GNUC__)
    #define typeof __typeof__
    #define BOOST_AUTO(Var, Expr) typeof(Expr) Var = Expr
#elif defined(_HAVE_CXX11_)
    #define BOOST_AUTO(Var, Expr) auto Var = Expr
#else
    #error typeof emulation is not supported
#endif

#ifndef __has_attribute
	#define HAS_ATTRIBUTE(x) 0
#else
	#define HAS_ATTRIBUTE(x) __has_attribute(x)
#endif

#ifndef __has_cpp_attribute
	#define HAS_CPP_ATTRIBUTE(x) 0
#else
	#define HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#endif

#ifndef __has_extension
	#define HAS_EXTENSION(x) 0
#else
	#define HAS_EXTENSION(x) __has_extension(x)
#endif

#if defined(__clang__) || defined(__GNUC__)
	#define ALIGNED(size) __attribute__((__aligned__(size)))
#elif defined(_MSC_VER)
	#define ALIGNED(size) __declspec(align(size))
#else
	#error Cannot define ALIGNED on this platform
#endif

/**
 * Fallthrough to indicate that `break` was left out on purpose in a switch
 * statement, e.g.
 *
 * switch (n) {
 *   case 22:
 *   case 33:  // no warning: no statements between case labels
 *     f();
 *   case 44:  // warning: unannotated fall-through
 *     g();
 *     FALLTHROUGH; // no warning: annotated fall-through
 * }
 */
#if HAS_CPP_ATTRIBUTE(clang::fallthrough)
    #define FALLTHROUGH [[clang::fallthrough]]
#else
    #define FALLTHROUGH
#endif

enum
{
    /// Memory locations on the same cache line are subject to false
    /// sharing, which is very bad for performance.  Microbenchmarks
    /// indicate that pairs of cache lines also see interference under
    /// heavy use of atomic operations (observed for atomic increment on
    /// Sandy Bridge).  See ALIGN_TO_AVOID_FALSE_SHARING
    kFalseSharingRange = 128
};

// TODO replace __attribute__ with alignas and 128 with kFalseSharingRange
/// An attribute that will cause a variable or field to be aligned so that
/// it doesn't have false sharing with anything at a smaller memory address.
#define ALIGN_TO_AVOID_FALSE_SHARING ALIGNED(128)

template <class To, class From>
inline To implicit_cast(From const& f)
{
    return f;
}

#endif // _PREPROCESSOR_H_ 

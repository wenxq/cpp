
#ifndef _TRAITS_H_
#define _TRAITS_H_

#include <memory>
#include <limits>
#include <type_traits>

// libc++ doesn't provide this header
#if !USE_LIBCPP
    // This file appears in two locations: inside fbcode and in the
    // libstdc++ source code (when embedding fbstring as std::string).
    // To aid in this schizophrenic use, two macros are defined in
    // c++config.h:
    //   _LIBSTDCXX_FBSTRING - Set inside libstdc++.  This is useful to
    //      gate use inside fbcode v. libstdc++
    #include <bits/c++config.h>
#endif

// Is T one of T1, T2, ..., Tn?
template <class T, class... Ts>
struct IsOneOf
{
    enum { value = false };
};

template <class T, class T1, class... Ts>
struct IsOneOf<T, T1, Ts...>
{
    enum { value = std::is_same<T, T1>::value || IsOneOf<T, Ts...>::value };
};

/*
 * Complementary type traits for integral comparisons.
 *
 * For instance, `if(x < 0)` yields an error in clang for unsigned types
 *  when -Werror is used due to -Wtautological-compare
 *
 *
 * @author: Marcelo Juchem <marcelo@fb.com>
 */

namespace detail {

template <typename T, bool>
struct is_negative_impl
{
    static bool check(T x)
    {
        return x < 0;
    }
};

template <typename T>
struct is_negative_impl<T, false>
{
    static bool check(T x)
    {
        return false;
    }
};

} // namespace detail

// same as `x < 0`
template <typename T>
bool is_negative(T x)
{
    return detail::is_negative_impl<T, std::is_signed<T>::value>::check(x);
}

// same as `x <= 0`
template <typename T>
bool is_non_positive(T x)
{
    return !x || is_negative(x);
}

// same as `x > 0`
template <typename T>
bool is_positive(T x)
{
    return !is_non_positive(x);
}

// same as `x >= 0`
template <typename T>
bool is_non_negative(T x)
{
    return !x || is_positive(x);
}

template <typename RHS, typename LHS>
bool less_than(LHS const lhs, RHS const rhs)
{
    if (rhs <= std::numeric_limits<LHS>::min())
    {
        return false;
    }
    else if (rhs > std::numeric_limits<LHS>::max())
    {
        return true;
    }
    else
    {
        return lhs < rhs;
    }
}

template <typename RHS, typename LHS>
bool greater_than(LHS const lhs, RHS const rhs)
{
    if (rhs < std::numeric_limits<LHS>::min())
    {
        return true;
    }
    else if (rhs > std::numeric_limits<LHS>::max())
    {
        return false;
    }
    else
    {
        return lhs > rhs;
    }
}

#endif //_TRAITS_H_

// Three-state boolean logic library

// Copyright Douglas Gregor 2002-2004. Use, modification and
// distribution is subject to the Boost Software License, Version
// 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


// For more information, see http://www.boost.org
#ifndef _TRIBOOL_H_
#define _TRIBOOL_H_

class tribool;

/// INTERNAL ONLY
namespace detail {
/**
 * INTERNAL ONLY
 *
 * \brief A type used only to uniquely identify the 'indeterminate'
 * function/keyword.
 */
struct indeterminate_t
{ };

} // end namespace detail

/**
 * INTERNAL ONLY
 * The type of the 'indeterminate' keyword. This has the same type as the
 * function 'indeterminate' so that we can recognize when the keyword is
 * used.
 */
typedef bool (*indeterminate_keyword_t)(tribool, detail::indeterminate_t);

/**
 * \brief Keyword and test function for the indeterminate tribool value
 *
 * The \c indeterminate function has a dual role. It's first role is
 * as a unary function that tells whether the tribool value is in the
 * "indeterminate" state. It's second role is as a keyword
 * representing the indeterminate (just like "true" and "false"
 * represent the true and false states). If you do not like the name
 * "indeterminate", and would prefer to use a different name, see the
 * macro \c BOOST_TRIBOOL_THIRD_STATE.
 *
 * \returns <tt>x.value == tribool::indeterminate_value</tt>
 * \throws nothrow
 */
inline bool
indeterminate(tribool x,
              detail::indeterminate_t dummy = detail::indeterminate_t()
             ) throw ();

/**
 * \brief A 3-state boolean type.
 *
 * 3-state boolean values are either true, false, or
 * indeterminate.
 */
class tribool
{
private:
    /// INTERNAL ONLY
    struct dummy
    {
        void nonnull() {};
    };

    typedef void (dummy::*safe_bool)();

public:
    /**
     * Construct a new 3-state boolean value with the value 'false'.
     *
     * \throws nothrow
     */
    tribool() throw ()
        : value(false_value)
    {}

    /**
     * Construct a new 3-state boolean value with the given boolean
     * value, which may be \c true or \c false.
     *
     * \throws nothrow
     */
    tribool(bool initial_value) throw ()
        : value(initial_value ? true_value : false_value)
    {}

    /**
     * Construct a new 3-state boolean value with an indeterminate value.
     *
     * \throws nothrow
     */
    tribool(indeterminate_keyword_t) throw ()
        : value(indeterminate_value)
    {}

    /**
     * Use a 3-state boolean in a boolean context. Will evaluate true in a
     * boolean context only when the 3-state boolean is definitely true.
     *
     * \returns true if the 3-state boolean is true, false otherwise
     * \throws nothrow
     */
    operator safe_bool() const throw ()
    {
        return value == true_value ? &dummy::nonnull : 0;
    }

    /**
     * The actual stored value in this 3-state boolean, which may be false, true,
     * or indeterminate.
     */
    enum value_t
    {
        false_value,
        true_value,
        indeterminate_value
    } value;
};

// Check if the given tribool has an indeterminate value. Also doubles as a
// keyword for the 'indeterminate' value
inline bool indeterminate(tribool x, detail::indeterminate_t) throw ()
{
    return x.value == tribool::indeterminate_value;
}

inline tribool operator!(tribool x) throw ()
{
    return x.value == tribool::false_value ? tribool(true)
           : x.value == tribool::true_value ? tribool(false)
           : tribool(indeterminate);
}

inline tribool operator&&(tribool x, tribool y) throw ()
{
    return (static_cast<bool>(!x) || static_cast<bool>(!y))
           ? tribool(false)
           : ((static_cast<bool>(x) && static_cast<bool>(y))
              ? tribool(true)
              : indeterminate)
           ;
}

/**
 * \overload
 */
inline tribool operator&&(tribool x, bool y) throw ()
{
    return y ? x : tribool(false);
}

/**
 * \overload
 */
inline tribool operator&&(bool x, tribool y) throw ()
{
    return x ? y : tribool(false);
}

/**
 * \overload
 */
inline tribool operator&&(indeterminate_keyword_t, tribool x) throw ()
{
    return !x ? tribool(false) : tribool(indeterminate);
}

/**
 * \overload
 */
inline tribool operator&&(tribool x, indeterminate_keyword_t) throw ()
{
    return !x ? tribool(false) : tribool(indeterminate);
}

inline tribool operator||(tribool x, tribool y) throw ()
{
    return (static_cast<bool>(!x) && static_cast<bool>(!y))
           ? tribool(false)
           : ((static_cast<bool>(x) || static_cast<bool>(y))
              ? tribool(true)
              : tribool(indeterminate))
           ;
}

/**
 * \overload
 */
inline tribool operator||(tribool x, bool y) throw ()
{
    return y ? tribool(true) : x;
}

/**
 * \overload
 */
inline tribool operator||(bool x, tribool y) throw ()
{
    return x ? tribool(true) : y;
}

/**
 * \overload
 */
inline tribool operator||(indeterminate_keyword_t, tribool x) throw ()
{
    return x ? tribool(true) : tribool(indeterminate);
}

/**
 * \overload
 */
inline tribool operator||(tribool x, indeterminate_keyword_t) throw ()
{
    return x ? tribool(true) : tribool(indeterminate);
}
//@}

inline tribool operator==(tribool x, tribool y) throw ()
{
    return (indeterminate(x) || indeterminate(y))
           ? indeterminate
           : ((x && y) || (!x && !y))
           ;
}

/**
 * \overload
 */
inline tribool operator==(tribool x, bool y) throw ()
{
    return x == tribool(y);
}

/**
 * \overload
 */
inline tribool operator==(bool x, tribool y) throw ()
{
    return tribool(x) == y;
}

/**
 * \overload
 */
inline tribool operator==(indeterminate_keyword_t, tribool x) throw ()
{
    return tribool(indeterminate) == x;
}

/**
 * \overload
 */
inline tribool operator==(tribool x, indeterminate_keyword_t) throw ()
{
    return tribool(indeterminate) == x;
}

inline tribool operator!=(tribool x, tribool y) throw ()
{
    return (indeterminate(x) || indeterminate(y))
           ? indeterminate
           : !((x && y) || (!x && !y))
           ;
}

/**
 * \overload
 */
inline tribool operator!=(tribool x, bool y) throw ()
{
    return x != tribool(y);
}

/**
 * \overload
 */
inline tribool operator!=(bool x, tribool y) throw ()
{
    return tribool(x) != y;
}

/**
 * \overload
 */
inline tribool operator!=(indeterminate_keyword_t, tribool x) throw ()
{
    return tribool(indeterminate) != x;
}

/**
 * \overload
 */
inline tribool operator!=(tribool x, indeterminate_keyword_t) throw ()
{
    return x != tribool(indeterminate);
}

/**
 * \brief Declare a new name for the third state of a tribool
 *
 * Use this macro to declare a new name for the third state of a
 * tribool. This state can have any number of new names (in addition
 * to \c indeterminate), all of which will be equivalent. The new name will be
 * placed in the namespace in which the macro is expanded.
 *
 * Example:
 *   BOOST_TRIBOOL_THIRD_STATE(true_or_false)
 *
 *   tribool x(true_or_false);
 *   // potentially set x
 *   if (true_or_false(x)) {
 *     // don't know what x is
 *   }
 */
#define BOOST_TRIBOOL_THIRD_STATE(Name) \
    inline bool \
    Name(tribool x, \
         detail::indeterminate_t = detail::indeterminate_t()) \
    { return x.value == tribool::indeterminate_value; }

#endif // _TRIBOOL_H_



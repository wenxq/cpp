// See http://www.boost.org/libs/any for Documentation.

#ifndef _ANY_H_
#define _ANY_H_

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Antony Polukhin, Ed Brey, Mark Rodgers, 
//        Peter Dimov, and James Curran
// when:  July 2001, April 2013 - May 2013

#include <algorithm>
#include <typeinfo>
#include "mpl.h"

class any
{
public: // structors

    any()
      : content(0)
    {
    }

    template<typename ValueType>
    any(const ValueType & value)
      : content(new holder<
            typename std::remove_cv<typename std::decay<const ValueType>::type>::type
        >(value))
    {
    }

    any(const any & other)
      : content(other.content ? other.content->clone() : 0)
    {
    }

    ~any()
    {
        delete content;
    }

public: // modifiers

    any & swap(any & rhs)
    {
        std::swap(content, rhs.content);
        return *this;
    }


    template<typename ValueType>
    any & operator=(const ValueType & rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

    any & operator=(any rhs)
    {
        any(rhs).swap(*this);
        return *this;
    }

public: // queries

    bool empty() const
    {
        return !content;
    }

    void clear()
    {
        any().swap(*this);
    }

    const std::type_info& type() const
    {
        return content ? content->type() : typeid(void);
    }

private: // types

    class placeholder
    {
    public: // structors

        virtual ~placeholder()
        {
        }

    public: // queries

        virtual const std::type_info& type() const = 0;

        virtual placeholder * clone() const = 0;

    };

    template<typename ValueType>
    class holder : public placeholder
    {
    public: // structors

        holder(const ValueType & value)
          : held(value)
        {
        }

    public: // queries

        virtual const std::type_info& type() const
        {
            return typeid(ValueType);
        }

        virtual placeholder * clone() const
        {
            return new holder(held);
        }

    public: // representation

        ValueType held;

    private: // intentionally left unimplemented
        holder & operator=(const holder &);
    };

private: // representation

    template<typename ValueType>
    friend ValueType * any_cast(any *);

    template<typename ValueType>
    friend ValueType * unsafe_any_cast(any *);

    placeholder * content;

};
 
inline void swap(any & lhs, any & rhs)
{
    lhs.swap(rhs);
}

class bad_any_cast :
    public std::bad_cast
{
public:
    virtual const char * what() const throw()
    {
        return "bad_any_cast: "
               "failed conversion using any_cast";
    }
};

template<typename ValueType>
ValueType * any_cast(any * operand)
{
    return operand && operand->type() == typeid(ValueType)
        ? &static_cast<any::holder<typename std::remove_cv<ValueType>::type> *>(operand->content)->held
        : 0;
}

template<typename ValueType>
inline const ValueType * any_cast(const any * operand)
{
    return any_cast<ValueType>(const_cast<any *>(operand));
}

template<typename ValueType>
ValueType any_cast(any & operand)
{
    typedef typename std::remove_reference<ValueType>::type nonref;

    nonref * result = any_cast<nonref>(&operand);
    if(!result)
        throw bad_any_cast();

    // Attempt to avoid construction of a temporary object in cases when 
    // `ValueType` is not a reference. Example:
    // `static_cast<std::string>(*result);` 
    // which is equal to `std::string(*result);`
    typedef typename mpl::if_<
        std::is_reference<ValueType>::value,
        ValueType,
        typename std::add_lvalue_reference<ValueType>::type
    >::type ref_type;

    return static_cast<ref_type>(*result);
}

template<typename ValueType>
inline ValueType any_cast(const any & operand)
{
    typedef typename std::remove_reference<ValueType>::type nonref;
    return any_cast<const nonref &>(const_cast<any &>(operand));
}


// Note: The "unsafe" versions of any_cast are not part of the
// public interface and may be removed at any time. They are
// required where we know what type is stored in the any and can't
// use typeid() comparison, e.g., when our types may travel across
// different shared libraries.
template<typename ValueType>
inline ValueType * unsafe_any_cast(any * operand)
{
    return &static_cast<any::holder<ValueType> *>(operand->content)->held;
}

template<typename ValueType>
inline const ValueType * unsafe_any_cast(const any * operand)
{
    return unsafe_any_cast<ValueType>(const_cast<any *>(operand));
}

// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#endif


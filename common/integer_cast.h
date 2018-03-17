
#ifndef _INTEGER_CAST_H_
#define _INTEGER_CAST_H_

#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>

#ifndef NDEBUG
    #define INTEGER_CAST_ACTION(x) do { throw detail::bad_cast(); } while (0)
#else
    #define INTEGER_CAST_ACTION(x) do { return (x); } while (0)
#endif // NDEBUG

namespace detail
{
    class bad_cast : public std::exception
    {
    public:
        virtual const char* what() const throw()
        {
            return "bad integer cast";
        }
    };

    template <class T, class U>
    inline T integer_cast_impl(const U& u,
                    const std::integral_constant<bool, true>& same_signed,
                    const std::integral_constant<bool, true>& size_le)
    {
        return static_cast<T>(u);
    }

    template <class T, class U>
    inline T integer_cast_impl(const U& u,
                    const std::integral_constant<bool, true>& same_signed,
                    const std::integral_constant<bool, false>& size_gt)
    {
        if (u > std::numeric_limits<T>::max())
        {
            INTEGER_CAST_ACTION(std::numeric_limits<T>::max());
        }
        else if (u < std::numeric_limits<T>::min())
        {
            INTEGER_CAST_ACTION(std::numeric_limits<T>::min());
        }

        return static_cast<T>(u);
    }

    template <class T, class U>
    inline T integer_cast_impl(const U& u,
                    const std::integral_constant<bool, false>& diff_signed,
                    const std::integral_constant<bool, true>&  size_le)
    {
        if (std::is_signed<U>::value && u < 0)
        {
            INTEGER_CAST_ACTION(std::numeric_limits<T>::min());
        }

        return static_cast<T>(u);
    }

    template <class T, class U>
    inline T integer_cast_impl_help(const U& u, const std::integral_constant<bool, true>& u_signed)
    {
        if (u < 0)
        {
            INTEGER_CAST_ACTION(std::numeric_limits<T>::min());
        }

        if (static_cast<typename std::make_unsigned<U>::type >(u) > std::numeric_limits<T>::max())
        {
            INTEGER_CAST_ACTION(std::numeric_limits<T>::max());
        }

        return static_cast<T>(u);
    }

    template <class T, class U>
    inline T integer_cast_impl_help(const U& u, const std::integral_constant<bool, false>& u_unsigned)
    {
        if (u > static_cast<typename std::make_unsigned<T>::type >(std::numeric_limits<T>::max()))
        {
            INTEGER_CAST_ACTION(std::numeric_limits<T>::max());
        }

        return static_cast<T>(u);
    }

    template <class T, class U>
    inline T integer_cast_impl(const U& u,
                    const std::integral_constant<bool, false>& diff_signed,
                    const std::integral_constant<bool, false>& size_gt)
    {
        return integer_cast_impl_help<T, U>(u, std::integral_constant<bool, std::is_signed<U>::value>());
    }

	template <class T, class U>
	struct is_same_signed
	{
		static const bool value = (std::is_unsigned<T>::value == std::is_unsigned<U>::value
								|| std::is_signed<T>::value == std::is_signed<U>::value);
	};

    template <class T, class U>
    inline T integer_cast(const U& u)
    {
        return integer_cast_impl<T, U>(
                   u,
                   std::integral_constant<bool, is_same_signed<T, U>::value>(),
                   std::integral_constant<bool, sizeof(U) <= sizeof(T)>()
            );
    }
} // namespace detail

using detail::integer_cast;

#endif // _INTEGER_CAST_H_


#ifndef _STRING_ALGO_H_
#define _STRING_ALGO_H_

#include "slice.h"
#include "base64.h"
#include <sstream>

int64_t atoi(Slice s);
int64_t hextoi(Slice s);
int64_t octtoi(Slice s);

struct is_any_of
{
    template <size_t N>
    is_any_of(const char (&s)[N])
      : mPiece(s, integer_cast<int>(N-1))
    {}

    is_any_of(const char* s, int len)
      : mPiece(s, len)
    {}

    is_any_of(const Slice& s)
      : mPiece(s)
    {}

    bool operator()(char c) const
    {
        return mPiece.find(c) != Slice::NPOS;
    }

    Slice mPiece;
};

struct is_none_of
{
    template <size_t N>
    is_none_of(const char (&s)[N])
      : mPiece(s, integer_cast<int>(N-1))
    {}

    is_none_of(const char* s, int len)
      : mPiece(s, len)
    {}

    bool operator()(char c) const
    {
        return mPiece.find(c) == Slice::NPOS;
    }

    Slice mPiece;
};

typedef std::pair<Slice, Slice> SlicePair;

namespace detail
{
    const uint8_t b2hex[] = "0123456789abcdef";

    // Converts a byte given as its hexadecimal representation
    // into a proper byte. Handles uppercase and lowercase letters
    // but does not check for overflows.
    inline uint8_t x2c(const uint8_t *what)
    {
        register int32_t digit;

        digit = (what[0] >= 'A' \
                    ? ((what[0] & 0xdf) - 'A') + 10 \
                    : (what[0] - '0'));

        digit <<= 4; // digit *= 16

        digit += (what[1] >= 'A' \
                    ? ((what[1] & 0xdf) - 'A') + 10 \
                    : (what[1] - '0'));

        return integer_cast<uint8_t>(digit);
    }

    // Converts a single byte into its hexadecimal representation.
    // Will overwrite two bytes at the destination.
    inline uint8_t* c2x(uint8_t what, uint8_t* where)
    {
        what &= 0xff;
        *where++ = b2hex[what >> 4];
        *where++ = b2hex[what & 0x0f];

        return where;
    }

    // Converts a single hexadecimal digit into a decimal value.
    inline uint8_t xsingle2c(const uint8_t *what)
    {
        register int32_t digit;

        digit = (what[0] >= 'A' \
                    ? ((what[0] & 0xdf) - 'A') + 10 \
                    : (what[0] - '0'));

        return integer_cast<uint8_t>(digit);
    }
} // endof namespace detail

template <class Container, class Separator>
inline size_t
split(Container& container, const Slice& piece, const Separator& sep, size_t maxsplit = Slice::npos)
{
    return piece.split(container, sep, maxsplit);
}

template <class Container, class Separator>
inline size_t split(Container& container, const std::string& str, const Separator& sep, size_t maxsplit = Slice::npos)
{
    return make_slice(str).split(container, sep, maxsplit);
}

// template <class Container>
// inline std::pair<size_t, std::string>
// split(Container& container, const std::string&& str, char sep, size_t maxsplit = Slice::npos)
// {
//     std::string store(str.data(), str.length());
//     size_t count = split(container, store, sep, maxsplit);
//     return std::pair<size_t, std::string>(count, std::move(store));
// }

template <class Container, size_t N>
inline size_t
split(Container& container, const char (&str)[N], char sep, size_t maxsplit = Slice::npos)
{
    return make_slice(str).split(container, sep, maxsplit);
}

template <class ApplyProxy, class Separator1, class Separator2>
size_t 
split_kv(const ApplyProxy& apply, const Slice& piece,
         const Separator1& key_sep, const Separator2& value_sep,
         size_t maxsplit = Slice::npos)
{
    size_t count = 0;
    int start = 0;
    do
    {
        int beg = piece.find(key_sep, start);
        if (beg == Slice::NPOS)
        {
            apply(piece.substr(start, piece.length() - start), Slice());
            ++count;
            break;
        }
        beg += detail::length_of(key_sep);

        int end = piece.find(value_sep, beg);
        if (end == Slice::NPOS)
            end = piece.length();

        apply(piece.substr(start, beg - detail::length_of(key_sep) - start),
              piece.substr(beg, end - beg));
        if (++count >= maxsplit)
            break;

        start = end + detail::length_of(value_sep);
    } while (start < piece.length());

    return count;
}

// template <class ApplyProxy, class Separator1, class Separator2>
// inline size_t
// split_kv(const ApplyProxy& apply, const std::string& str,
//          const Separator1& key_sep, const Separator2& value_sep,
//          size_t maxsplit = Slice::npos)
// {
//     return split_kv(apply, make_slice(str), key_sep, value_sep, maxsplit);
// }
//
// template <class ApplyProxy, class Separator1, class Separator2>
// inline std::pair<size_t, std::string>
// split_kv(ApplyProxy&& apply, std::string&& str,
//          const Separator1& key_sep, const Separator2& value_sep,
//          size_t maxsplit = Slice::npos)
// {
//     std::string store(str.data(), str.length());
//     size_t count = split_kv(apply, make_slice(store), key_sep, value_sep, maxsplit);
//     return std::pair<size_t, std::string>(count, std::move(store));
// }

template <class ApplyProxy, class Separator1, class Separator2, size_t N>
inline size_t
split_kv(const ApplyProxy& apply, const char (&str)[N],
         const Separator1& key_sep, const Separator2& value_sep,
         size_t maxsplit = Slice::npos)
{
    return split_kv(apply, make_slice(str), key_sep, value_sep, maxsplit);
}

bool mark_first_hp(const Slice& substr, int* bs, uint32_t len);
int  find_first_hp(const Slice& str, const Slice& substr, const int* bs, uint32_t len);
template <size_t N>
inline bool mark_first_hp(const Slice& substr, int (&bs)[N])
{
    return mark_first_hp(substr, bs, N); 
}
template <size_t N>
inline int find_first_hp(const Slice& str, const Slice& substr, const int (&bs)[N])
{
    return find_first_hp(str, substr, bs, N); 
}

bool mark_first_hpcase(const Slice& substr, int* bs, uint32_t len);
int  find_first_hpcase(const Slice& str, const Slice& substr, const int* bs, uint32_t len);
template <size_t N>
inline bool mark_first_hpcase(const Slice& substr, int (&bs)[N])
{
    return mark_first_hpcase(substr, bs, N); 
}
template <size_t N>
inline int find_first_hpcase(const Slice& str, const Slice& substr, const int (&bs)[N])
{
    return find_first_hpcase(str, substr, bs, N); 
}

bool mark_last_hp(const Slice& substr, int* bs, uint32_t len);
int  find_last_hp(const Slice& str, const Slice& substr, const int* bs, uint32_t len);
template <size_t N>
inline bool mark_last_hp(const Slice& substr, int (&bs)[N])
{
    return mark_last_hp(substr, bs, N); 
}
template <size_t N>
inline int find_last_hp(const Slice& str, const Slice& substr, const int (&bs)[N])
{
    return find_last_hp(str, substr, bs, N); 
}

bool mark_last_hpcase(const Slice& substr, int *bs, uint32_t len);
int  find_last_hpcase(const Slice& str, const Slice& substr, const int* bs, uint32_t len);
template <size_t N>
inline bool mark_last_hpcase(const Slice& substr, int (&bs)[N])
{
    return mark_last_hpcase(substr, bs, N); 
}
template <size_t N>
inline int find_last_hpcase(const Slice& str, const Slice& substr, const int (&bs)[N])
{
    return find_last_hpcase(str, substr, bs, N); 
}

bool mark_first_kmp(const Slice& substr, int* next, uint32_t len);
int  find_first_kmp(const Slice& str, const Slice& substr, const int* next);
template <size_t N>
inline bool mark_first_kmp(const Slice& substr, int (&bs)[N])
{
    return mark_first_kmp(substr, bs, N); 
}
template <size_t N>
inline int find_first_kmp(const Slice& str, const Slice& substr, const int (&bs)[N])
{
    return find_first_kmp(str, substr, bs, N); 
}

bool mark_first_kmpcase(const Slice& substr, int* next, uint32_t len);
int  find_first_kmpcase(const Slice& str, const Slice& substr, const int* next);
template <size_t N>
inline bool mark_first_kmpcase(const Slice& substr, int (&bs)[N])
{
    return mark_first_kmpcase(substr, bs, N); 
}
template <size_t N>
inline int find_first_kmpcase(const Slice& str, const Slice& substr, const int (&bs)[N])
{
    return find_first_kmpcase(str, substr, bs, N); 
}

bool mark_first_wm(const Slice& substr, uint64_t* bs, uint32_t len);
int  find_first_wm(const Slice& str, const Slice& substr, const uint64_t* bs, uint32_t len);
template <size_t N>
inline bool mark_first_wm(const Slice& substr, uint64_t (&bs)[N])
{
    return mark_first_wm(substr, bs, N); 
}
template <size_t N>
inline int find_first_wm(const Slice& str, const Slice& substr, const uint64_t (&bs)[N])
{
    return find_first_wm(str, substr, bs, N); 
}

inline bool base64_encode(const Slice& input, std::string& output)
{
    output.resize(base64_encode_len(input.length()));  // makes room for null byte

    // null terminates result since result is base64 text!
    int output_size= base64_encode(&(output[0]), input.data(), input.length());
    if (output_size < 0)
        return false;

    output.resize(output_size);  // strips off null byte
    return true;
}

inline bool base64_decode(const Slice& input, std::string& output)
{
    output.resize(base64_decode_len(input.length()));

    // does not null terminate result since result is binary data!
    int output_size = base64_decode(&(output[0]), input.data(), input.length());
    if (output_size < 0)
        return false;

    output.resize(output_size);
    return true;
}

#endif // _STRING_ALGO_H_

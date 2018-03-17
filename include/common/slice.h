// Copyright 2010-2014 Google
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// A std::string-like object that points to a sized piece of memory.
//
// Functions or methods may use const Slice& parameters to accept either
// a "const char*" or a "std::string" value that will be implicitly converted to
// a Slice.  The implicit conversion means that it is often appropriate
// to include this .h file in other files rather than forward-declaring
// Slice as would be appropriate for most other Google classes.
//
// Systematic usage of Slice is encouraged as it will reduce unnecessary
// conversions from "const char*" to "std::string" and back again.

#ifndef _SLICE_H_
#define _SLICE_H_

#include "preprocessor.h"
#include "smart_assert.h"
#include "integer_cast.h"
#include "tools.h"

#ifdef _MSC_VER
#define strcasecmp  _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

#include <cassert>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iosfwd>
#include <string>
#include <utility>

#define slice_requires_string_len(_String, _Len) \
     SMART_ASSERT(_String != nullptr || _Len > 0) \
                  ("length", _Len)("pointer", (uintptr_t)(_String))

class Slice
{
public:
    // Standard STL container boilerplate.
    typedef char        value_type;
    typedef const char* pointer;
    typedef const char& reference;
    typedef const char& const_reference;
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef const char* const_iterator;
    typedef const char* iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    static const size_type npos = static_cast<size_type>(-1);
    static const int NPOS = -1;

    // We provide non-explicit singleton constructors so users can pass
    // in a "const char[N]" or a "std::string" wherever a "Slice" is
    // expected.
    Slice() : ptr_(nullptr), length_(0) {}

    template <size_t N>
    Slice(const char (&str)[N])
      : ptr_(str)
      , length_(integer_cast<int>(N-1))
    {}

    Slice(const std::string& str) // NOLINT
      : ptr_(str.data())
      , length_(integer_cast<int>(str.size()))
    {}

    Slice(const char* str)
      : ptr_(str)
      , length_(str ? strlen(str) : 0)
    {}

    Slice(const char* offset, int len)
      : ptr_(offset)
      , length_(len)
    {
        assert((length_ > 0 && ptr_ != nullptr) || (length_ == 0)); //&& ptr_ == nullptr));
    }

    Slice(const char* beg, const char* end)
      : ptr_(beg)
      , length_(integer_cast<int>(end - beg))
    {
        assert((length_ > 0 && ptr_ != nullptr) || (length_ == 0)); //&& ptr_ == nullptr));
    }

    iterator begin() const { return ptr_; }
    iterator end()   const { return ptr_ + length_; }

    reverse_iterator rbegin() const
    {
        return reverse_iterator(ptr_ + length_);
    }

    reverse_iterator rend() const
    {
        return reverse_iterator(ptr_);
    }

    const_iterator cbegin() const { return ptr_; }
    const_iterator cend()   const { return ptr_ + length_; }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(ptr_ + length_);
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(ptr_);
    }

    const char& front() const { return *ptr_; }
    const char& back()  const { return ptr_[length_ - 1]; }

    // STL says return size_type, but Google says return int.
    int max_size() const { return length_; }
    int capacity() const { return length_; }

    int find(char c, size_type pos = 0) const;
    template <size_t N>
    int find(const char (&s)[N], size_type pos = 0) const
    {
        return find(s, integer_cast<int>(N-1), pos);
    }
    int find(const Slice& s, size_type pos = 0) const
    {
        return find(s.ptr_, s.length_, pos);
    }

    int rfind(char c, size_type pos = Slice::npos) const;
    template <size_t N>
    int rfind(const char (&s)[N], size_type pos = Slice::npos) const
    {
        return rfind(s, integer_cast<int>(N-1), pos);
    }
    int rfind(const Slice& s, size_type pos = Slice::npos) const
    {
        return rfind(s.ptr_, s.length_, pos);
    }

    int find_first(char c, size_type pos = 0) const
    {
        return find(c, pos);
    }
    template <size_t N>
    int find_first(const char (&s)[N], size_type pos = 0) const
    {
        return find(s, integer_cast<int>(N-1), pos);
    }
    int find_first(const Slice& s, size_type pos = 0) const
    {
        return find(s.ptr_, s.length_, pos);
    }

    int find_last(char c, size_type pos = Slice::npos) const;
    template <size_t N>
    int find_last(const char (&s)[N], size_type pos = Slice::npos) const
    {
        return find_last(s, integer_cast<int>(N-1), pos);
    }
    int find_last(const Slice& s, size_type pos = Slice::npos) const
    {
        return find_last(s.ptr_, s.length_, pos);
    }

    template <size_t N>
    int find_first_of(const char (&s)[N], size_type pos = 0) const
    {
        return find_first_of(s, integer_cast<int>(N-1), pos);
    }
    int find_first_of(const Slice& s, size_type pos = 0) const
    {
        return find_first_of(s.ptr_, s.length_, pos);
    }

    template <size_t N>
    int find_last_of(const char (&s)[N], size_type pos = Slice::npos) const
    {
        return find_last_of(s, integer_cast<int>(N-1), pos);
    }
    int find_last_of(const Slice& s, size_type pos = Slice::npos) const
    {
        return find_last_of(s.ptr_, s.length_, pos);
    }

    int find_first_not_of(char c, size_type pos = 0) const;
    template <size_t N>
    int find_first_not_of(const char (&s)[N], size_type pos = 0) const
    {
        return find_first_not_of(s, integer_cast<int>(N-1), pos);
    }
    int find_first_not_of(const Slice& s, size_type pos = 0) const
    {
        return find_first_not_of(s.ptr_, s.length_, pos);
    }

    int find_last_not_of(char c, size_type pos = Slice::npos) const;
    template <size_t N>
    int find_last_not_of(const char (&s)[N], size_type pos = Slice::npos) const
    {
        return find_last_not_of(s, integer_cast<int>(N-1), pos); 
    }
    int find_last_not_of(const Slice& s, size_type pos = Slice::npos) const
    {
        return find_last_not_of(s.ptr_, s.length_, pos);
    }

    // data() may return a pointer to a buffer with embedded NULs, and the
    // returned buffer may or may not be null terminated.  Therefore it is
    // typically a mistake to pass data() to a routine that expects a NUL
    // terminated std::string.
    pointer data()   const { return ptr_; }
    int     size()   const { return length_; }
    int     length() const { return length_; }
    bool    empty()  const { return length_ == 0; }

    char operator[](size_type i) const { return ptr_[i]; }

    void clear()
    {
        ptr_ = NULL;
        length_ = 0;
    }

    std::string to_string() const
    {
        return std::string(ptr_, length_);
    }

    int compare(const char* s, int len) const;
    int compare(const Slice& s) const
    {
        return compare(s.ptr_, s.length_);
    }

    int icompare(const char* s, int len) const;
    int icompare(const Slice& s) const
    {
        return icompare(s.ptr_, s.length_);
    }

    int copy(char* buf, size_type n, size_type pos = 0) const;

    Slice substr(size_type pos, size_type n = Slice::npos) const
    {
        assert(length_ >= 0);
        const size_type len = integer_cast<size_type>(length_);

        if (pos > len)
            pos = len;

        if (n > len - pos)
            n = len - pos;

        return Slice(ptr_ + pos, integer_cast<int>(n));
    }

    void trim()
    {
        trim_left();
        trim_right();
    }

    void trim_left();
    void trim_right();

    template <class Predicate>
    void trim_left_if(Predicate pred)
    {
        pointer last = ptr_ + length_;
        for (pointer pos = ptr_; pos != last; ++pos)
        {
            if (!pred(*pos))
            {
                length_ -= integer_cast<int>(pos - ptr_);
                ptr_ = pos;
                break;
            }
        }
    }

    template <class Predicate>
    void trim_right_if(Predicate pred)
    {
        for (pointer pos = ptr_ + (length_ - 1); pos >= ptr_; --pos)
        {
            if (!pred(*pos))
            {
                length_ = integer_cast<int>((pos - ptr_) + 1);
                break;
            }
        }
    }

    void reset()
    {
        ptr_ = nullptr;
        length_ = 0;
    }

    template <size_t N>
    void reset(const char (&data)[N])
    {
        ptr_ = data;
        length_ = integer_cast<int>(N-1);
    }

    void reset(const char* data, int len)
    {
        assert((len > 0 && data != nullptr) || (len == 0 && data == nullptr));
        ptr_ = data;
        length_ = len;
    }

    void reset(const char* start, const char* end)
    {
        assert((end - start > 0 && start != nullptr) || (end == start && start == nullptr));
        ptr_ = start;
        length_ = integer_cast<int>(end - start);
    }

    void reset(const void* data, int len)
    {
        assert((len > 0 && data != nullptr) || (len == 0 && data == nullptr));
        ptr_ = reinterpret_cast<const char*>(data);
        length_ = len;
    }

    void pop_front()
    {
        remove_prefix(1);
    }

    void advance(int n)
    {
        remove_prefix(n);
    }

    void remove_prefix(int n)
    {
        assert(n >= 0 && n <= length_);
        ptr_ += n;
        length_ -= n;
    }

    void pop_back()
    {
        remove_suffix(1);
    }

    void remove_suffix(int n)
    {
        assert(n >= 0 && n <= length_);
        length_ -= n;
    }

    void copy_to(std::string* target) const
    {
        if (!empty())
        {
            target->assign(ptr_, length_);
        }
        else
        {
            target->clear();
        }
    }

    void append_to(std::string* target) const
    {
        if (!empty())
        {
            target->append(ptr_, length_);
        }
    }

    bool starts_with(const Slice& s) const
    {
        return ((length_ >= s.length_) && (memcmp(ptr_, s.ptr_, s.length_) == 0));
    }
    bool starts_with(const char* s, int len) const
    {
        slice_requires_string_len(s, len);
        return ((length_ >= len) && (memcmp(ptr_, s, len) == 0));
    }

    bool ends_with(const Slice& s) const
    {
        return ((length_ >= s.length_) &&
                (memcmp(ptr_ + (length_ - s.length_), s.ptr_, s.length_) == 0));
    }
    bool ends_with(const char* s, int len) const
    {
        slice_requires_string_len(s, len);
        return ((length_ >= len) &&
                (memcmp(ptr_ + (length_ - len), s, len) == 0));
    }

    bool istarts_with(const Slice& s) const
    {
        return ((length_ >= s.length_) && (strncasecmp(ptr_, s.ptr_, s.length_) == 0));
    }
    bool istarts_with(const char* s, int len) const
    {
        slice_requires_string_len(s, len);
        return ((length_ >= len) && (strncasecmp(ptr_, s, len) == 0));
    }

    bool iends_with(const Slice& s) const
    {
        return ((length_ >= s.length_) &&
                (strncasecmp(ptr_ + (length_ - s.length_), s.ptr_, s.length_) == 0));
    }
    bool iends_with(const char* s, int len) const
    {
        slice_requires_string_len(s, len);
        return ((length_ >= len) &&
                (strncasecmp(ptr_ + (length_ - len), s, len) == 0));
    }

    template <class Container>
    size_type split(Container& container, size_t maxsplit = Slice::npos) const
    {
        size_type count = 0;
        pointer last = ptr_ + length_;
        pointer prev = ptr_;
        while (prev != last && isspace(static_cast<uint8_t>(*prev)))
            ++prev;
        for (pointer p = prev; p < last; ++p)
        {
            if (isspace(static_cast<uint8_t>(*p)))
            {
                container.push_back(Slice(prev, integer_cast<int>(p - prev)));
                if (++count >= maxsplit)
                    break;
                while (++p != last && isspace(static_cast<uint8_t>(*p)))
                    ; // nothing
                prev = p--;
            }
            else if (p + 1 == last)
            {
                container.push_back(Slice(prev, integer_cast<int>(last - prev)));
                ++count;
            }
        }
        return count;
    }

    template <class Container>
    size_type split(Container& container, char sep, size_t maxsplit = Slice::npos) const
    {
        size_type count = 0;
        pointer last = ptr_ + length_;
        pointer prev = ptr_;
        while (prev != last && sep == *prev)
            ++prev;
        for (pointer p = prev; p < last; ++p)
        {
            if (sep == *p)
            {
                container.push_back(Slice(prev, integer_cast<int>(p - prev)));
                prev = p + 1;
                if (++count >= maxsplit)
                    break;
                while (++p != last && sep == *p)
                    ; // nothing
                prev = p--;
            }
            else if (p + 1 == last)
            {
                container.push_back(Slice(prev, integer_cast<int>(last - prev)));
                ++count;
            }
        }
        return count;
    }

    template <class Container, class Separator>
    size_type split(Container& container, const Separator& sep, size_t maxsplit = Slice::npos) const
    {
        const int len = detail::length_of(sep);
        size_t count = 0;
        int start = 0;
        do
        {
            int pos = find(sep, start);
            if (pos == Slice::NPOS)
            {
                container.push_back(substr(start, length_ - start));
                ++count;
                break;
            }

            container.push_back(substr(start, pos - start));
            if (++count >= maxsplit)
                break;

            start = pos + len;
        } while (start < length_);

        return count;
    }

private:
    int find(const char* s, int len, size_type pos) const;
    int rfind(const char* s, int len, size_type pos) const;
    int find_last(const char* s, int len, size_type pos) const;
    int find_first_of(const char* s, int len, size_type pos) const;
    int find_last_of(const char* s, int len, size_type pos) const;
    int find_first_not_of(const char* s, int len, size_type pos) const;
    int find_last_not_of(const char* s, int len, size_type pos) const;

private:
    pointer ptr_;
    int     length_;
};

inline bool operator==(const Slice& x, const Slice& y)
{
    if (x.size() != y.size()) return false;

    if (x.size() > 0 && x.back() != y.back())
        return false;

    return memcmp(x.data(), y.data(), x.size()) == 0;
}

inline bool operator!=(const Slice& x, const Slice& y)
{
    return !(x == y);
}

inline bool operator<(const Slice& x, const Slice& y)
{
    const int r = memcmp(x.data(), y.data(), std::min(x.size(), y.size()));
    return ((r < 0) || ((r == 0) && (x.size() < y.size())));
}

inline bool operator>(const Slice& x, const Slice& y)
{
    return y < x;
}

inline bool operator<=(const Slice& x, const Slice& y)
{
    return !(x > y);
}

inline bool operator>=(const Slice& x, const Slice& y)
{
    return !(x < y);
}

inline std::string to_string(Slice piece)
{
    return piece.to_string();
}

namespace detail
{
    inline int length_of(const Slice& x)
    {
        return x.length();
    }

} // namespace detail

// Allow Slice to be logged.
std::ostream& operator<<(std::ostream& o, const Slice& piece);
inline std::ostream& operator<<(std::ostream& o, const Slice* piece)
{
    return (o << (*piece));
}

template <size_t N>
inline Slice make_slice(const char (&str)[N])
{
    return Slice(str);
}

inline Slice make_slice(const std::string& str)
{
    return Slice(str.data(), detail::integer_cast<int>(str.size()));
}

inline Slice make_slice(const void* data, int len)
{
    return Slice(reinterpret_cast<const char*>(data), len);
}

inline Slice make_slice(const void* beg, const void* end)
{
    return Slice(reinterpret_cast<const char*>(beg), reinterpret_cast<const char*>(end));
}

#endif  // _SLICE_H_

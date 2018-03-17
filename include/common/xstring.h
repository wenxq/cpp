
#ifndef _XSTRING_H_
#define _XSTRING_H_

#include "slice.h"

#include <ctype.h>
#include <cstdlib>
#include <cstdio>

#include <exception>
#include <limits>

#define _XSTRING_INIT_(data, len) do { \
    SMART_ASSERT((len) >= 0); \
    if ((len) > capacity()) { \
        size_t nsize = integer_cast<size_t>(len) * 2; \
        if (nsize > integer_cast<size_t>(max_size() / 3)) { \
            nsize = integer_cast<size_t>(len); \
        } \
        mData = reinterpret_cast<pointer>(malloc(nsize)); \
        mSize = nsize; \
    } \
    if (len != 0) memcpy(mData, data, len); \
    mPos = mData + (len); \
    mData[mSize - 1] = '\0'; \
} while (0);

#ifdef _MSC_VER
#   define I64_FMT "%lld"
#   define U64_FMT "%llu"
#else
#   define I64_FMT "%ld"
#   define U64_FMT "%lu"
#endif // _MSC_VER

template <size_t SZ>
class XString
{
public:
    typedef char        value_type;
    typedef char*       pointer;
    typedef const char* const_pointer;
    typedef char&       reference;
    typedef const char& const_reference;
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef char*       iterator;
    typedef const char* const_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

    static const size_type npos = static_cast<size_type>(-1);
    static const int NPOS = -1;

    XString()
      : mData(mStore)
      , mPos(mStore)
      , mSize(dimensionof(mStore))
    {}

    template <size_t N>
    XString(const char (&str)[N])
      : mData(mStore)
      , mPos(mStore)
      , mSize(dimensionof(mStore))
    {
        _XSTRING_INIT_(str, integer_cast<int>(N-1));
    }

    XString(const Slice& piece)
      : mData(mStore)
      , mPos(mStore)
      , mSize(dimensionof(mStore))
    {
        _XSTRING_INIT_(piece.data(), piece.length());
    }

    XString(const char* offset, int len)
      : mData(mStore)
      , mPos(mStore)
      , mSize(dimensionof(mStore))
    {
        _XSTRING_INIT_(offset, len);
    }

    XString(const char* beg, const char* end)
      : mData(mStore)
      , mPos(mStore)
      , mSize(dimensionof(mStore))
    {
        _XSTRING_INIT_(beg, integer_cast<int>(end - beg));
    }

    XString(const XString& other)
      : mData(mStore)
      , mPos(mStore)
      , mSize(dimensionof(mStore))
    {
        _XSTRING_INIT_(other.mData, other.length());
    }

    XString& operator=(const XString& other)
    {
        return assign(other.mData, other.mPos);
    }

    template <size_t N>
    XString& operator=(const char (&str)[N])
    {
        return assign(str, integer_cast<int>(N-1));
    }

    XString& operator=(const Slice& piece)
    {
        return assign(piece.data(), piece.length());
    }

    ~XString()
    {
        if (mData != mStore)
            free(mData);
    }

    int  size()     const { return integer_cast<int>(mPos - mData); }
    int  length()   const { return integer_cast<int>(mPos - mData); }
    bool empty()    const { return mPos == mData;                   }
    int  capacity() const { return integer_cast<int>(mSize - 1);    }
    int  max_size() const { return std::numeric_limits<int>::max(); }

    const_iterator cbegin() const { return mData; }
    const_iterator cend()   const { return mPos;  }

    const_reverse_iterator crbegin() const
    {
        return const_reverse_iterator(mPos);
    }

    const_reverse_iterator crend() const
    {
        return const_reverse_iterator(mData);
    }

    char front() const { return *mData;   }
    char back()  const { return mPos[integer_cast<int>(-1)]; }

    const_pointer data() const { return mData; }

    char operator[](size_type i) const { return mData[i]; }

    iterator begin()  { return mData; }
    iterator end()    { return mPos;  }

    reverse_iterator rbegin()
    {
        return reverse_iterator(mPos);
    }

    reverse_iterator rend()
    {
        return reverse_iterator(mData);
    }

    char& front()  { return *mData;   }
    char& back()   { return mPos[integer_cast<int>(-1)]; }

    pointer data() { return mData;  }

    char& operator[](size_type i) { return mData[i]; }

    const_pointer c_str() const
    {
        *mPos = '\0';
        return mData;
    }

    void clear()
    {
        mPos = mData;
    }

    void push_back(char c)
    {
        if (length() + 1 > capacity())
            _reserve(compute_next_size(1), true);
        *mPos++ = c;
    }

    void push_back(uint8_t c)
    {
        if (length() + 1 > capacity())
            _reserve(compute_next_size(1), true);
        *mPos++ = c;
    }

    void push_back(int16_t i)
    {
        if (length() + 6 + 1 > capacity())
            _reserve(compute_next_size(6 + 1), true);
        int len = snprintf(mPos, 6, "%d", i);
        SMART_ASSERT(len > 0);
        mPos += len;
    }

    void push_back(int32_t i)
    {
        if (length() + 12 + 1 > capacity())
            _reserve(compute_next_size(12 + 1), true);
        int len = snprintf(mPos, 12, "%d", i);
        SMART_ASSERT(len > 0);
        mPos += len;
    }

    void push_back(int64_t i)
    {
        if (length() + 20 + 1 > capacity())
            _reserve(compute_next_size(20 + 1), true);
        int len = snprintf(mPos, 20, I64_FMT, i);
        SMART_ASSERT(len > 0);
        mPos += len;
    }

    void push_back(uint16_t i)
    {
        if (length() + 6 + 1 > capacity())
            _reserve(compute_next_size(6 + 1), true);
        int len = snprintf(mPos, 6, "%u", i);
        SMART_ASSERT(len > 0);
        mPos += len;
    }

    void push_back(uint32_t i)
    {
        if (length() + 12 + 1 > capacity())
            _reserve(compute_next_size(12 + 1), true);
        int len = snprintf(mPos, 12, "%u", i);
        SMART_ASSERT(len > 0);
        mPos += len;
    }

    void push_back(uint64_t i)
    {
        if (length() + 20 + 1 > capacity())
            _reserve(compute_next_size(20 + 1), true);
        int len = snprintf(mPos, 20, U64_FMT, i);
        SMART_ASSERT(len > 0);
        mPos += len;
    }

    XString& append(const_pointer data, int len)
    {
        SMART_ASSERT(len >= 0);

        if (len <= 0) return *this;

        void* to_free = nullptr;
        if (length() + len > capacity())
        {
            to_free = _reserve(compute_next_size(len),
                                mData == mStore || data < begin() || data >= end());
        }
        memcpy(mPos, data, len);
        mPos += len;
        if (to_free) free(to_free);
        return *this;
    }

    XString& append(const_pointer beg, const_pointer end)
    {
        return append(beg, integer_cast<int>(end - beg));
    }

    template <size_t N>
    XString& append(const XString<N>& str)
    {
        return append(str.cbegin(), str.cend());
    }

    template <size_t N>
    XString& append(const char (&str)[N])
    {
        return append(str, integer_cast<int>(N-1));
    }

    XString& append(const Slice& str)
    {
        return append(str.data(), str.length());
    }

    template <size_t N>
    XString& operator+=(const XString<N>& str)
    {
        return append(str.cbegin(), str.cend());
    }

    template <size_t N>
    XString& operator+=(const char (&str)[N])
    {
        return append(str, integer_cast<int>(N-1));
    }

    XString& operator+=(const Slice& piece)
    {
        return append(piece.data(), piece.length());
    }

    XString& assign(const_pointer data, int len)
    {
        SMART_ASSERT(len >= 0);
        if (len <= 0)
        {
            mPos = mData;
            return *this;
        }
        else if (data == mData)
        {
            SMART_ASSERT(len <= capacity());
            mPos = mData + len;
            return *this;
        }
        else if (data > mData && data < mData + mSize)
        {
            SMART_ASSERT(data < mPos && data + len <= mPos);
            memmove(mData, data, len);
            mPos = mData + len;
            return *this;
        }

        if (integer_cast<size_type>(len) < dimensionof(mStore))
        {
            if (mData != mStore)
                free(mData);
            mData = mStore;
            mSize = dimensionof(mStore);
        }
        else if (len > capacity())
        {
            _reserve(len, true);
        }
        memcpy(mData, data, len);
        mPos = mData + len;
        return *this;
    }

    XString& assign(const_pointer beg, const_pointer end)
    {
        return assign(beg, integer_cast<int>(end - beg));
    }

    template <size_t N>
    XString& assign(const XString<N>& str)
    {
        return assign(str.cbegin(), str.cend());
    }

    template <size_t N>
    XString& assign(const char (&str)[N])
    {
        return assign(str, integer_cast<int>(N-1));
    }

    XString& assign(const Slice& str)
    {
        return assign(str.data(), str.length());
    }

    void reserve(int nsize)
    {
        if (nsize > capacity())
        {
            _reserve(nsize, true);
        }
    }

    void resize(int nsize)
    {
        if (nsize > capacity())
        {
            _reserve(nsize, true);
        }

        mPos = mData + nsize;
    }

    std::string to_string() const
    {
        return std::string(mData, mPos - mData);
    }

    Slice substr(size_type pos, size_type n = XString::npos) const
    {
        const size_type len = integer_cast<size_type>(length());

        if (pos > len)
            pos = len;

        if (n > len - pos)
            n = len - pos;

        return Slice(mData + pos, integer_cast<int>(n));
    }

    operator Slice() const
    {
        return Slice(mData, mPos);
    }

    XString& tolower();
    XString& toupper();

    int compare(const char* s, int len) const;

    template <size_t N>
    int compare(const XString<N>& str) const
    {
        return compare(str.data(), str.length());
    }

    template <size_t N>
    int compare(const char (&str)[N]) const
    {
        return compare(str, integer_cast<int>(N-1));
    }
    int compare(const Slice& s) const
    {
        return compare(s.data(), s.length());
    }

    int icompare(const char* s, int len) const;

    template <size_t N>
    int icompare(const XString<N>& str) const
    {
        return icompare(str.data(), str.length());
    }

    template <size_t N>
    int icompare(const char (&str)[N]) const
    {
        return icompare(str, integer_cast<int>(N-1));
    }
    int icompare(const Slice& s) const
    {
        return icompare(s.data(), s.length());
    }

    void swap(XString& other);

private:
    size_t compute_next_size(int n) const
    {
        const int nsize = size();
        if (n > max_size() - nsize)
        {
            throw std::bad_alloc();
        }

        size_t len = integer_cast<size_t>(nsize) + integer_cast<size_t>(std::max(n, nsize)) * 2 + 1;
        if (len > integer_cast<size_t>(max_size()) || len < integer_cast<size_t>(nsize))
        {
            len = max_size(); // overflow
        }
        return len;
    }

    void* _reserve(size_t nsize, bool diff)
    {
        if (nsize > integer_cast<size_t>(max_size()))
            throw std::bad_alloc();

        void* to_free = nullptr;
        void* ptr = (mData == mStore) ? nullptr : mData;
        if (diff)
        {
            ptr = realloc(ptr, nsize);
            if (ptr == nullptr)
                throw std::bad_alloc();

            if (mData == mStore)
                memcpy(ptr, mStore, size());
        }
        else
        {
            ptr = malloc(nsize);
            if (ptr == nullptr)
                throw std::bad_alloc();

            memcpy(ptr, mData, size());
            if (mData != mStore)
                to_free = mData;
        }

        int len = length();
        mData = reinterpret_cast<pointer>(ptr);
        mPos  = mData + len;
        mSize = nsize;
        mData[mSize - 1] = '\0';

        return to_free;
    }

private:
    pointer mData;
    pointer mPos;
    size_t  mSize;
    char    mStore[SZ];
};

template <size_t N>
inline int XString<N>::compare(const char* s, int len) const
{
    slice_requires_string_len(s, len);
    int r = memcmp(mData, s, std::min(length(), len));
    if (r == 0)
    {
        if (length() < len)
            r = -1;
        else if (length() > len)
            r = +1;
    }
    return r;
}

template <size_t N>
inline int XString<N>::icompare(const char* s, int len) const
{
    slice_requires_string_len(s, len);
    int r = strncasecmp(mData, s, std::min(length(), len));
    if (r == 0)
    {
        if (length() < len)
            r = -1;
        else if (length() > len)
            r = +1;
    }
    return r;
}

template <size_t N>
inline XString<N>& XString<N>::tolower()
{
    for (pointer xpos = mData; xpos != mPos; ++xpos)
    {
        *xpos = static_cast<char>(::tolower(*xpos));
    }
    return *this;
}

template <size_t N>
inline XString<N>& XString<N>::toupper()
{
    for (pointer xpos = mData; xpos != mPos; ++xpos)
    {
        *xpos = static_cast<char>(::toupper(*xpos));
    }
    return *this;
}

template <size_t N>
inline void XString<N>::swap(XString<N>& other)
{
    bool h1 = (mData != mStore);
    bool h2 = (other.mData != other.mStore);

    if (h1 && h2)
    {
        std::swap(mData, other.mData);
        std::swap(mPos,  other.mPos);
        std::swap(mSize, other.mSize);
    }
    else if (!h1 && !h2)
    {
        const int nsize = size();
        char tmp[dimensionof(mStore)];
        memcpy(tmp, mStore, nsize);
        memcpy(mStore, other.mStore, other.size());
        memcpy(other.mStore, tmp, nsize);

        mPos = mData + other.size();
        other.mPos = other.mData + nsize;
    }
    else if (h1 && !h2)
    {
        const int nsize = other.size();
        memcpy(mStore, other.mStore, nsize);
        other.mData = mData;
        other.mPos  = mPos;
        mData = mStore;
        mPos  = mData + nsize;
    }
    else if (!h1 && h2)
    {
        const int nsize = size();
        memcpy(other.mStore, mStore, nsize);
        mData = other.mData;
        mPos  = other.mPos;
        other.mData = other.mStore;
        other.mPos  = other.mData + nsize;
    }
}

template <size_t N>
inline std::string to_string(const XString<N>& str)
{
    return str.to_string();
}

// Allow CString to be logged.
template <size_t N>
inline std::ostream& operator<<(std::ostream& o, const XString<N>& s)
{
    o.write(s.data(), s.size());
    return o;
}

template <size_t N>
inline Slice make_slice(const XString<N>& str)
{
    return Slice(str.data(), str.length());
}

template <size_t N>
inline Slice make_slice(const XString<N>* str)
{
    return Slice(str->data(), str->length());
}

typedef XString<512> StackString;

#undef I64_FMT
#undef U64_FMT

#endif // _XSTRING_H_

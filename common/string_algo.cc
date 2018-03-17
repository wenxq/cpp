
#include "string_algo.h"
#include "string-inl.h"
#include <limits>

#define CHAR_CASE_EQ(a, b)  (ToLower[static_cast<uint8_t>(a)] == ToLower[static_cast<uint8_t>(b)])
#define CHAR_CASE_NEQ(a, b) (ToLower[static_cast<uint8_t>(a)] != ToLower[static_cast<uint8_t>(b)])

typedef Slice::pointer pointer;

namespace detail {

    const int64_t cutoff10 = std::numeric_limits<int64_t>::max() / 10;
    const int64_t cutlim   = std::numeric_limits<int64_t>::max() % 10;

    const int64_t cutoff8  = std::numeric_limits<int64_t>::max() / 8;
    const int64_t cutoff16 = std::numeric_limits<int64_t>::max() / 16;

} // namespace detail

int64_t atoi(Slice s)
{
    s.trim_left();

    if (s.empty()) return 0;

    char sign = '+'; 
    const pointer last = s.end();
    pointer xpos = s.begin();
    if (*xpos == '-' || *xpos == '+')
    {
        sign = *xpos;
        ++xpos;
    }

    int64_t value = 0;
    for ( ; xpos != last; ++xpos)
    {
        if (*xpos < '0' || *xpos > '9')
            break;

        if (value >= detail::cutoff10 && (value > detail::cutoff10 || (*xpos - '0') > detail::cutlim))
            return 0;

        value = value * 10 + (*xpos - '0');
    }

    return (sign == '-') ? -value : value;
}

int64_t hextoi(Slice s)
{
    s.trim_left();

    if (s.empty()) return 0;

    const pointer last = s.end();
    int64_t value = 0;
    for (pointer xpos = s.begin(); xpos != last; ++xpos)
    {
        if (value > detail::cutoff16)
            return 0;

        if (*xpos >= '0' && *xpos <= '9')
        {
            value = value * 16 + (*xpos - '0');
            continue;
        }

        uint8_t c = (uint8_t) (*xpos | 0x20);
        if (c >= 'a' && c <= 'f')
        {
            value = value * 16 + (c - 'a' + 10);
            continue;
        }

        return 0;
    }

    return value;
}

int64_t octtoi(Slice s)
{
    s.trim_left();

    if (s.empty()) return 0;

    const pointer last = s.end();
    int64_t value = 0;
    for (pointer xpos = s.begin(); xpos != last; ++xpos)
    {
        if (value > detail::cutoff8)
            return 0;

        if (*xpos >= '0' && *xpos <= '7')
        {
            value = value * 8 + (*xpos - '0');
            continue;
        }

        return 0;
    }

    return value;
}

bool mark_first_hp(const Slice& substr, int* bm, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
        bm[i] = substr.length();

    pointer last = substr.end() - 1;
    for (pointer xpos = substr.begin(); xpos != last; ++xpos)
    {
        uint8_t c = static_cast<uint8_t>(*xpos);
        if (c >= len) return false;
        bm[c] = integer_cast<uint32_t>(last - xpos);
    }
    return true;
}

int find_first_hp(const Slice& str, const Slice& substr, const int* bm, uint32_t len)
{
    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;
    
    if (str.length() == substr.length())
        return memcmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    pointer last = str.end() - substr.length();
    for (pointer pos = str.begin(); pos <= last; )
    {
        pointer epos = pos + (substr.length() - 1);
        if (*epos == substr.back() && memcmp(pos, substr.data(), substr.length()-1) == 0)
            return integer_cast<int>(pos - str.begin());

        uint8_t c = static_cast<uint8_t>(*epos);
        pos += (c >= len) ? substr.length() : bm[c];
    }
    return Slice::NPOS;
}

bool mark_first_hpcase(const Slice& substr, int* bm, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
        bm[i] = substr.length();

    pointer last = substr.end() - 1;
    for (pointer xpos = substr.begin(); xpos != last; ++xpos)
    {
        uint8_t c = static_cast<uint8_t>(*xpos);
        if (c >= len) return false;
        bm[ToUpper[c]] = integer_cast<uint32_t>(last - xpos);
        bm[ToLower[c]] = integer_cast<uint32_t>(last - xpos);
    }
    return true;
}

int find_first_hpcase(const Slice& str, const Slice& substr, const int* bm, uint32_t len)
{
    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;
    
    if (str.length() == substr.length())
        return strncasecmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    pointer last = str.end() - substr.length();
    for (pointer pos = str.begin(); pos <= last; )
    {
        pointer epos = pos + (substr.length() - 1);
		if (CHAR_CASE_EQ(*epos, substr.back()))
		{
			if (strncasecmp(pos, substr.data(), substr.length() - 1) == 0)
				return integer_cast<int>(pos - str.begin());
		}

        uint8_t c = static_cast<uint8_t>(*epos);
        pos += (c >= len) ? substr.length() : bm[c];
    }
    return Slice::NPOS;
}

bool mark_last_hp(const Slice& substr, int *bm, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
        bm[i] = substr.length();

    for (pointer xpos = substr.end() - 1; xpos != substr.begin(); --xpos)
    {
        uint8_t c = static_cast<uint8_t>(*xpos);
        if (c >= len) return false;
        bm[c] = integer_cast<uint32_t>(xpos - substr.begin());
    }
    return true;
}

int find_last_hp(const Slice& str, const Slice& substr, const int* bm, uint32_t len)
{
    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;
    
    if (str.length() == substr.length())
        return memcmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    for (pointer epos = str.end() - substr.length(); epos >= str.begin(); )
    {
        if (*epos == substr.front() && memcmp(epos+1, substr.data()+1, substr.length()-1) == 0)
            return integer_cast<int>(epos - str.begin());

        uint8_t c = static_cast<uint8_t>(*epos);
        epos -= (c >= len) ? substr.length() : bm[c];
    }
    return Slice::NPOS;
}

bool mark_last_hpcase(const Slice& substr, int *bm, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i)
        bm[i] = substr.length();

    for (pointer xpos = substr.end() - 1; xpos != substr.begin(); --xpos)
    {
        uint8_t c = static_cast<uint8_t>(*xpos);
        if (c >= len) return false;
        bm[ToUpper[c]] = integer_cast<uint32_t>(xpos - substr.begin());
        bm[ToLower[c]] = integer_cast<uint32_t>(xpos - substr.begin());
    }
    return true;
}

int find_last_hpcase(const Slice& str, const Slice& substr, const int* bm, uint32_t len)
{
    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;
    
    if (str.length() == substr.length())
        return strncasecmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    for (pointer epos = str.end() - substr.length(); epos >= str.begin(); )
    {
        if (CHAR_CASE_EQ(*epos, substr.front()) && strncasecmp(epos+1, substr.data()+1, substr.length()-1) == 0)
            return integer_cast<int>(epos - str.begin());

        uint8_t c = static_cast<uint8_t>(*epos);
        epos -= (c >= len) ? substr.length() : bm[c];
    }
    return Slice::NPOS;
}

bool mark_first_kmp(const Slice& substr, int* next, uint32_t len)
{
    if (integer_cast<int32_t>(len) - 1 < substr.length())
        return false;

    uint32_t i = 0;
    int    j = -1;
    next[i]  = -1;
    while (i < len)
    {
        while (j > -1 && substr[i] != substr[j])
            j = next[j];

        next[++i] = ++j;
    }

    return true;
}

int find_first_kmp(const Slice& str, const Slice& substr, const int* next)
{
    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;

    if (str.length() == substr.length())
        return memcmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    int idx = 0;
    const pointer last = str.end();
    for (pointer xpos = str.begin(); xpos <= last; ++xpos)
    {
        while (idx != -1 && *xpos != substr[idx])
            idx = next[idx];

        if (++idx == substr.length())
            return integer_cast<int>(xpos - str.begin()) + 1 - substr.length();
    }

    return Slice::NPOS;
}

bool mark_first_kmpcase(const Slice& substr, int* next, uint32_t len)
{
    if (integer_cast<int32_t>(len) - 1 < substr.length())
        return false;

    uint32_t i = 0;
    int    j = -1;
    next[i]  = -1;
    while (i < len)
    {
        while (j > -1 && CHAR_CASE_NEQ(substr[i], substr[j]))
        {
            j = next[j];
        }

        next[++i] = ++j;
    }

    return true;
}

int find_first_kmpcase(const Slice& str, const Slice& substr, const int* next)
{
    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;

    if (str.length() == substr.length())
        return strncasecmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    int idx = 0;
    const pointer last = str.end();
    for (pointer xpos = str.begin(); xpos <= last; ++xpos)
    {
        while (idx != -1 && CHAR_CASE_NEQ(*xpos, substr[idx]))
            idx = next[idx];

        if (++idx == substr.length())
            return integer_cast<int>(xpos - str.begin()) + 1 - substr.length();
    }

    return Slice::NPOS;
}

bool mark_first_wm(const Slice& substr, uint64_t* bs, uint32_t len)
{
    if (substr.length() > 64)
        return false;

    bzero(bs, sizeof(uint64_t) * len);

    uint64_t j = 1;
    for (pointer xpos = substr.begin(); xpos != substr.end(); ++xpos)
    {
        uint8_t c = static_cast<uint8_t>(*xpos);
        if (c >= len) return false;

        bs[c] |= j;

        j <<= 1;
    }

    return true;
}

int find_first_wm(const Slice& str, const Slice& substr, const uint64_t* bs, uint32_t len)
{
    SMART_ASSERT(substr.length() <= 64);

    if (str.length() < substr.length() || substr.length() == 0)
        return Slice::NPOS;
    
    if (str.length() == substr.length())
        return memcmp(str.data(), substr.data(), substr.length()) == 0 ? 0 : Slice::NPOS;

    const uint64_t match = static_cast<uint64_t>(1) << (substr.length() - 1);
    uint64_t state = 0;
    for (pointer xpos = str.begin(); xpos != str.end(); ++xpos)
    {
        uint8_t c = static_cast<uint8_t>(*xpos);
        state = ((state << 1) | static_cast<uint64_t>(1)) & (c >= len ? static_cast<uint64_t>(0) : bs[c]);
        if ((match & state) != 0)
            return integer_cast<int>(xpos - str.begin()) - substr.length() + 1;
    }

    return Slice::NPOS;
}


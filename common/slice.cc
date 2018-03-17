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

#include "slice.h"
#include <iostream>  // NOLINT

std::ostream& operator<<(std::ostream& o, const Slice& piece)
{
    o.write(piece.data(), piece.size());
    return o;
}

int Slice::copy(char* buf, size_type n, size_type pos) const
{
    slice_requires_string_len(buf, n);
    if (length_ <= 0) return 0;
    assert(pos < integer_cast<size_type>(length_));
    int ret = std::min(length_ - integer_cast<int>(pos), integer_cast<int>(n));
    memcpy(buf, ptr_ + pos, ret);
    return ret;
}

int Slice::compare(const char* s, int len) const
{
    slice_requires_string_len(s, len);
    int r = memcmp(ptr_, s, std::min(length_, len));
    if (r == 0)
    {
        if (length_ < len)
            r = -1;
        else if (length_ > len)
            r = +1;
    }
    return r;
}

int Slice::icompare(const char* s, int len) const
{
    slice_requires_string_len(s, len);
    int r = strncasecmp(ptr_, s, std::min(length_, len));
    if (r == 0)
    {
        if (length_ < len)
            r = -1;
        else if (length_ > len)
            r = +1;
    }
    return r;
}

int Slice::find(char c, size_type pos) const
{
    assert(length_ >= 0);
    if (pos >= integer_cast<size_type>(length_))
        return Slice::NPOS;

    pointer last = ptr_ + length_;
    for (pointer xpos = ptr_ + pos; xpos < last; ++xpos)
    {
        if (*xpos == c)
            return integer_cast<int>(xpos - ptr_);
    }

    return Slice::NPOS;
}

int Slice::find(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    if (length_ < len || pos > integer_cast<Slice::size_type>(length_ - len))
        return Slice::NPOS;

    pointer last = ptr_ + length_;
    for (pointer xpos = ptr_ + pos; xpos != last; ++xpos)
    {
        if (memcmp(xpos, s, len) == 0)
            return integer_cast<int>(xpos - ptr_);
    }

    return Slice::NPOS;
}

int Slice::rfind(char c, size_type pos) const
{
    if (length_ <= 0) return Slice::NPOS;
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_ - 1));
    for ( ; xpos >= ptr_; --xpos)
    {
        if (c == *xpos)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

int Slice::rfind(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    if (length_ <= 0 || length_ < len)
        return Slice::NPOS;

    pointer last = ptr_ + std::min(integer_cast<size_type>(length_ - len), pos) + len;
    for ( ; last >= ptr_; --last)
    {
        if (memcmp(last, s, len) == 0)
            return integer_cast<int>(last - ptr_);
    }

    return Slice::NPOS;
}

int Slice::find_last(char c, size_type pos) const
{
    if (length_ <= 0) return Slice::NPOS;
    const size_type epos = integer_cast<size_type>(length_ - 1);
    pointer last = (pos > epos) ? ptr_ : (ptr_ + pos);
    for (pointer xpos = ptr_ + epos; xpos >= last; --xpos)
    {
        if (c == *xpos)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

int Slice::find_last(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    if (length_ <= 0 || length_ < len)
        return Slice::NPOS;

    const size_type epos = integer_cast<size_type>(length_ - len);
    pointer last = (pos > epos) ? ptr_ : (ptr_ + pos);
    for (pointer xpos = ptr_ + epos; xpos >= last; --xpos)
    {
        if (memcmp(xpos, s, len) == 0)
            return integer_cast<int>(xpos - ptr_);
    }

    return Slice::NPOS;
}

int Slice::find_first_of(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_));
    pointer last = ptr_ + length_;
    for (pointer slast = s + len; xpos != last; ++xpos)
    {
        pointer result = std::find(s, slast, *xpos);
        if (result != slast)
            return integer_cast<int>(xpos - ptr_);
    }

    return Slice::NPOS;
}

int Slice::find_last_of(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    if (length_ <= 0) return Slice::NPOS;
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_ - 1));
    for (pointer slast = s + len; xpos >= ptr_; --xpos)
    {
        pointer result = std::find(s, slast, *xpos);
        if (result != slast)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

int Slice::find_first_not_of(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    pointer last = ptr_ + length_;
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_));
    for (pointer slast = s + len; xpos != last; ++xpos)
    {
        pointer result = std::find(s, slast, *xpos);
        if (result == slast)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

int Slice::find_first_not_of(char c, size_type pos) const
{
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_));
    for (pointer last = ptr_ + length_; xpos != last; ++xpos)
    {
        if (c != *xpos)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

int Slice::find_last_not_of(const char* s, int len, size_type pos) const
{
    slice_requires_string_len(s, len);
    if (length_ <= 0) return Slice::NPOS;
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_ - 1));
    for (pointer slast = s + len; xpos >= ptr_; --xpos)
    {
        pointer result = std::find(s, slast, *xpos);
        if (result == slast)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

int Slice::find_last_not_of(char c, size_type pos) const
{
    if (length_ <= 0) return Slice::NPOS;
    pointer xpos = ptr_ + std::min(pos, integer_cast<size_type>(length_ - 1));
    for ( ; xpos >= ptr_; --xpos)
    {
        if (c != *xpos)
            return integer_cast<int>(xpos - ptr_);
    }
    return Slice::NPOS;
}

void Slice::trim_left()
{
    if (empty()) return ;

    pointer last = ptr_ + length_;
    for (pointer pos = ptr_; pos != last; ++pos)
    {
        if (!isspace(static_cast<uint8_t>(*pos)))
        {
            length_ -= integer_cast<int>(pos - ptr_);
            ptr_ = pos;
            break;
        }
        else if (pos + 1 == last)
        {
            length_ = 0;
            break;
        }
    }
}

void Slice::trim_right()
{
    if (empty()) return ;

    for (pointer pos = ptr_ + (length_ - 1); pos >= ptr_; --pos)
    {
        if (!isspace(static_cast<uint8_t>(*pos)))
        {
            length_ = integer_cast<int>(pos - ptr_) + 1;
            break;
        }
        else if (pos == ptr_)
        {
            length_ = 0;
            break;
        }
    }
}


#ifndef _UTILS_H_
#define _UTILS_H_

#include "interface.h"
#include <cstdlib>
#include <string>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#ifdef _MSC_VER
#  define PATH_SEP '\\'
#else
#  define PATH_SEP '/'
#endif // _MSC_VER

namespace detail
{
	inline std::string md5_string(const u_char (&m)[16])
	{
		std::string desc;

		char buf[3];
		for (size_t i = 0; i < 16; ++i)
		{
			snprintf(buf, 3, "%.02x", m[i]);
			desc.append(buf);
		}

		return desc;
	}
} // namespace detail

template <class C, class T>
bool contains(const C& c, const T& t)
{
	return !c.empty() && (c.find(t) != c.end());
}

std::string md5sum(const char* filename);

bool IsASNI(const Slice& str);
bool IsUTF8(const Slice& str);
Slice GetFileName(const Slice& utf8, std::vector<char>& buffer);
Slice DecodeFileName(const Slice& filename, std::vector<char>& buffer);

#endif // _UTILS_H_

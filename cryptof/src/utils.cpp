
#include "utils.h"
#include "exception.h"
#include "common/slice.h"
#include "cryptopp/md5.h"
#include <fstream>

#ifdef _MSC_VER
#include <windows.h>
#endif // _MSC_VER

std::string md5sum(const char* filename)
{
	CryptoPP::Weak::MD5 md5;

	char line[1024];
	std::ifstream in(filename, std::ios::in | std::ios::binary);
	while (in.read(line, sizeof(line)))
	{
		md5.Update((const byte *)line, integer_cast<int>(in.gcount()));
	}

	if (in.eof() && in.gcount() > 0 && in.gcount() < sizeof(line))
	{
		md5.Update((const byte *)line, integer_cast<int>(in.gcount()));
	}

	byte m[16];
	md5.Final(m);
	return detail::md5_string(m);
}

#define CHECK_UTF8(beg, end, count) do { \
    if (end - beg < count) return false; \
    const uint8_t* e = (beg++) + count; \
    while (beg < e) { \
        if (((*beg) & 0xc0) != 0x80) { \
        	return false; \
        } \
        ++beg; \
    } \
} while (0)

bool IsASNI(const Slice& str)
{
	const uint8_t* beg = (const uint8_t *)str.begin();
	const uint8_t* end = (const uint8_t *)str.end();

	while (beg != end)
	{
		if (*beg == 0 || *beg >= 128)
			return false;
		++beg;
	}

	return true;
}

bool IsUTF8(const Slice& str)
{
	const uint8_t* beg = (const uint8_t *)str.begin();
	const uint8_t* end = (const uint8_t *)str.end();

	while (beg != end)
	{
		SMART_ASSERT(beg < end)("is utf8 vaild", 1);
		if ((*beg & 0xfe) == 0xfe)
		{
			return false;
		}
		else if ((*beg & 0xfc) == 0xfc)
		{
			CHECK_UTF8(beg, end, 6);
		}
		else if ((*beg & 0xf8) == 0xf8)
		{
			CHECK_UTF8(beg, end, 5);
		}
		else if ((*beg & 0xf0) == 0xf0)
		{
			CHECK_UTF8(beg, end, 4);
		}
		else if ((*beg & 0xe0) == 0xe0)
		{
			CHECK_UTF8(beg, end, 3);
		}
		else if ((*beg & 0xc0) == 0xc0)
		{
			CHECK_UTF8(beg, end, 2);
		}
		else if ((*beg & 0x80) == 0x80)
		{
			return false;
		}
		else
		{
			++beg;
		}
	}

	return true;
}

#ifdef _MSC_VER

#define CODE_PAGE CP_ACP /* CP_OEMCP */

Slice MBytesToUTF8(const Slice& mbs, std::vector<char>& utf8)
{
	// mbytes -> unicode
	const int code_page = CODE_PAGE;
	int bytes = MultiByteToWideChar(code_page, 0, mbs.data(), mbs.size(), NULL, 0);
	if (bytes <= 0)
	{
		throw Exception("MultiByteToWideChar3 failed");
	}

	std::vector<WCHAR> unicode;
	unicode.resize(bytes + 1);
	LPWSTR wide = (LPWSTR)&unicode[0];
	int out = MultiByteToWideChar(code_page, 0, mbs.data(), mbs.size(), wide, bytes);
	if (bytes != out)
	{
		std::cout << "bytes:" << bytes << ", out:" << out << std::endl;
		throw Exception("MultiByteToWideChar4 failed");
	}

	// unicode -> utf8
	int mbytes = WideCharToMultiByte(CP_UTF8, 0, wide, bytes, NULL, 0, NULL, NULL);
	if (mbytes <= 0)
	{
		throw Exception("WideCharToMultiByte5 failed");
	}

	utf8.resize(mbytes + 1);
	LPSTR nowide = (LPSTR)&utf8[0];
	out = WideCharToMultiByte(CP_UTF8, 0, wide, bytes, nowide, mbytes, NULL, NULL);
	if (mbytes != out)
	{
		std::cerr << "mbytes:" << mbytes << ", out:" << out << std::endl;
		throw Exception("WideCharToMultiByte6 failed");
	}
	nowide[out] = '\0';

	return make_slice(nowide, mbytes);
}

Slice UTF8ToMBytes(const Slice& utf8, std::vector<char>& mbs)
{
	// utf8 -> unicode
	int bytes = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), NULL, 0);
	if (bytes <= 0)
	{
		throw Exception("MultiByteToWideChar1 failed");
	}

	std::vector<WCHAR> unicode;
	unicode.resize(bytes + 1);
	LPWSTR wide = (LPWSTR)&unicode[0];
	int out = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size(), wide, bytes);
	if (bytes != out)
	{
		std::cout << "bytes:" << bytes << ", out:" << out << std::endl;
		throw Exception("MultiByteToWideChar2 failed");
	}

	// unicode -> mbytes
	const int code_page = CODE_PAGE;
	int mbytes = WideCharToMultiByte(code_page, 0, wide, bytes, NULL, 0, NULL, NULL);
	if (mbytes <= 0)
	{
		throw Exception("WideCharToMultiByte1 failed");
	}

	mbs.resize(mbytes + 1);
	LPSTR nowide = (LPSTR)&mbs[0];
	out = WideCharToMultiByte(code_page, 0, wide, bytes, nowide, mbytes, NULL, NULL);
	if (mbytes != out)
	{
		std::cerr << "mbytes:" << mbytes << ", out:" << out << std::endl;
		throw Exception("WideCharToMultiByte2 failed");
	}
	nowide[out] = '\0';
	return make_slice(nowide, mbytes);
}

Slice GetFileName(const Slice& str, std::vector<char>& buffer)
{
	if (IsASNI(str))
	{
		return str;
	}
	else if (IsUTF8(str))
	{
		return UTF8ToMBytes(str, buffer);
	}

	std::cerr << "GetFileName unknown code(as mbytes)" << std::endl;
	return str;
}

Slice DecodeFileName(const Slice& str, std::vector<char>& buffer)
{
	if (IsASNI(str) || IsUTF8(str))
	{
		return str;
	}

	std::cerr << "DecodeFileName unknown code(as mbytes)" << std::endl;
	return MBytesToUTF8(str, buffer);
}

#else
Slice GetFileName(const Slice& utf8, std::vector<char>&)
{
	return utf8;
}

Slice DecodeFileName(const Slice& filename, std::vector<char>&)
{
	return filename;
}
#endif // _MSC_VER

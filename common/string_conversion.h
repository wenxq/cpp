
#ifndef COMMON_STRING_CONVERSION_H__
#define COMMON_STRING_CONVERSION_H__

#include <string>
#include <vector>

// Convert |in| to UTF-16 into |out|.  Use platform byte ordering.  If the
// conversion failed, |out| will be zero length.
void UTF8ToUTF16(const char* in, std::vector<uint16_t>* out);

// Convert at least one character (up to a maximum of |in_length|) from |in|
// to UTF-16 into |out|.  Return the number of characters consumed from |in|.
// Any unused characters in |out| will be initialized to 0.  No memory will
// be allocated by this routine.
int UTF8ToUTF16Char(const char* in, int in_length, uint16_t out[2]);

// Convert |in| to UTF-16 into |out|.  Use platform byte ordering.  If the
// conversion failed, |out| will be zero length.
void UTF32ToUTF16(const wchar_t* in, std::vector<uint16_t>* out);

// Convert |in| to UTF-16 into |out|.  Any unused characters in |out| will be
// initialized to 0.  No memory will be allocated by this routine.
void UTF32ToUTF16Char(wchar_t in, uint16_t out[2]);

// Convert |in| to UTF-8.  If |swap| is true, swap bytes before converting.
std::string UTF16ToUTF8(const std::vector<uint16_t>& in, bool swap);

#endif  // COMMON_STRING_CONVERSION_H__

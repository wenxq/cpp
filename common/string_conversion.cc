
#include <string.h>

#include "convert_UTF.h"
#include "scoped_ptr.h"
#include "string_conversion.h"

void UTF8ToUTF16(const char* in, std::vector<uint16_t>* out)
{
    size_t source_length = strlen(in);
    const UTF8* source_ptr = reinterpret_cast<const UTF8*>(in);
    const UTF8* source_end_ptr = source_ptr + source_length;
    // Erase the contents and zero fill to the expected size
    out->clear();
    out->insert(out->begin(), source_length, 0);
    uint16_t* target_ptr = &(*out)[0];
    uint16_t* target_end_ptr = target_ptr + out->capacity() * sizeof(uint16_t);
    ConversionResult result = ConvertUTF8toUTF16(&source_ptr, source_end_ptr,
                              &target_ptr, target_end_ptr,
                              strictConversion);

    // Resize to be the size of the # of converted characters + NULL
    out->resize(result == conversionOK ? target_ptr - & (*out)[0] + 1 : 0);
}

int UTF8ToUTF16Char(const char* in, int in_length, uint16_t out[2])
{
    const UTF8* source_ptr = reinterpret_cast<const UTF8*>(in);
    const UTF8* source_end_ptr = source_ptr + sizeof(char);
    uint16_t* target_ptr = out;
    uint16_t* target_end_ptr = target_ptr + 2 * sizeof(uint16_t);
    out[0] = out[1] = 0;

    // Process one character at a time
    while (1)
    {
        ConversionResult result = ConvertUTF8toUTF16(&source_ptr, source_end_ptr,
                                  &target_ptr, target_end_ptr,
                                  strictConversion);

        if (result == conversionOK)
            return static_cast<int>(source_ptr - reinterpret_cast<const UTF8*>(in));

        // Add another character to the input stream and try again
        source_ptr = reinterpret_cast<const UTF8*>(in);
        ++source_end_ptr;

        if (source_end_ptr > reinterpret_cast<const UTF8*>(in) + in_length)
            break;
    }

    return 0;
}

void UTF32ToUTF16(const wchar_t* in, std::vector<uint16_t>* out)
{
    size_t source_length = wcslen(in);
    const UTF32* source_ptr = reinterpret_cast<const UTF32*>(in);
    const UTF32* source_end_ptr = source_ptr + source_length;
    // Erase the contents and zero fill to the expected size
    out->clear();
    out->insert(out->begin(), source_length, 0);
    uint16_t* target_ptr = &(*out)[0];
    uint16_t* target_end_ptr = target_ptr + out->capacity() * sizeof(uint16_t);
    ConversionResult result = ConvertUTF32toUTF16(&source_ptr, source_end_ptr,
                              &target_ptr, target_end_ptr,
                              strictConversion);

    // Resize to be the size of the # of converted characters + NULL
    out->resize(result == conversionOK ? target_ptr - & (*out)[0] + 1 : 0);
}

void UTF32ToUTF16Char(wchar_t in, uint16_t out[2])
{
    const UTF32* source_ptr = reinterpret_cast<const UTF32*>(&in);
    const UTF32* source_end_ptr = source_ptr + 1;
    uint16_t* target_ptr = out;
    uint16_t* target_end_ptr = target_ptr + 2 * sizeof(uint16_t);
    out[0] = out[1] = 0;
    ConversionResult result = ConvertUTF32toUTF16(&source_ptr, source_end_ptr,
                              &target_ptr, target_end_ptr,
                              strictConversion);

    if (result != conversionOK)
    {
        out[0] = out[1] = 0;
    }
}

static inline uint16_t Swap(uint16_t value)
{
    return (value >> 8) | static_cast<uint16_t>(value << 8);
}

std::string UTF16ToUTF8(const std::vector<uint16_t>& in, bool swap)
{
    const UTF16* source_ptr = &in[0];
    scoped_array<uint16_t> source_buffer;

    // If we're to swap, we need to make a local copy and swap each byte pair
    if (swap)
    {
        int idx = 0;
        source_buffer.reset(new uint16_t[in.size()]);
        UTF16* source_buffer_ptr = source_buffer.get();
        for (std::vector<uint16_t>::const_iterator it = in.begin();
             it != in.end(); ++it, ++idx)
            source_buffer_ptr[idx] = Swap(*it);

        source_ptr = source_buffer.get();
    }

    // The maximum expansion would be 4x the size of the input string.
    const UTF16* source_end_ptr = source_ptr + in.size();
    size_t target_capacity = in.size() * 4;
    scoped_array<UTF8> target_buffer(new UTF8[target_capacity]);
    UTF8* target_ptr = target_buffer.get();
    UTF8* target_end_ptr = target_ptr + target_capacity;
    ConversionResult result = ConvertUTF16toUTF8(&source_ptr, source_end_ptr,
                              &target_ptr, target_end_ptr,
                              strictConversion);

    if (result == conversionOK)
    {
        const char* targetPtr = reinterpret_cast<const char*>(target_buffer.get());
        return targetPtr;
    }

    return "";
}


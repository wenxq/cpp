
#ifndef COMMON_WINDOWS_GUID_STRING_H_
#define COMMON_WINDOWS_GUID_STRING_H_

#include <guiddef.h>

#include <string>

class GUIDString
{
public:
    // Converts guid to a string in the format recommended by RFC 4122 and
    // returns the string.
    static std::wstring GUIDToWString(GUID* guid);

    // Converts guid to a string formatted as uppercase hexadecimal, with
    // no separators, and returns the string.  This is the format used for
    // symbol server identifiers, although identifiers have an age tacked
    // on to the string.
    static std::wstring GUIDToSymbolServerWString(GUID* guid);
};

#endif  // COMMON_WINDOWS_GUID_STRING_H_

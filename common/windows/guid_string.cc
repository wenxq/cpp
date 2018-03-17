
#include <wchar.h>
#include "common/windows/string_utils-inl.h"
#include "common/windows/guid_string.h"

// static
std::wstring GUIDString::GUIDToWString(GUID* guid)
{
    wchar_t guid_string[37];
    swprintf(
        guid_string, sizeof(guid_string) / sizeof(guid_string[0]),
        L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2],
        guid->Data4[3], guid->Data4[4], guid->Data4[5],
        guid->Data4[6], guid->Data4[7]);

    // remove when VC++7.1 is no longer supported
    guid_string[sizeof(guid_string) / sizeof(guid_string[0]) - 1] = L'\0';

    return std::wstring(guid_string);
}

// static
std::wstring GUIDString::GUIDToSymbolServerWString(GUID* guid)
{
    wchar_t guid_string[33];
    swprintf(
        guid_string, sizeof(guid_string) / sizeof(guid_string[0]),
        L"%08X%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
        guid->Data1, guid->Data2, guid->Data3,
        guid->Data4[0], guid->Data4[1], guid->Data4[2],
        guid->Data4[3], guid->Data4[4], guid->Data4[5],
        guid->Data4[6], guid->Data4[7]);

    // remove when VC++7.1 is no longer supported
    guid_string[sizeof(guid_string) / sizeof(guid_string[0]) - 1] = L'\0';

    return std::wstring(guid_string);
}

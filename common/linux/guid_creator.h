
#ifndef COMMON_LINUX_GUID_CREATOR_H__
#define COMMON_LINUX_GUID_CREATOR_H__

#include <stdint.h>

typedef struct {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t  data4[8];
} GUID;

// Format string for parsing GUID.
#define kGUIDFormatString "%08x-%04x-%04x-%08x-%08x"

// Length of GUID string. Don't count the ending '\0'.
#define kGUIDStringLength 36

// Create a guid.
bool CreateGUID(GUID* guid);

// Get the string from guid.
bool GUIDToString(const GUID* guid, char* buf, int buf_len);

#endif // COMMON_LINUX_GUID_CREATOR_H__

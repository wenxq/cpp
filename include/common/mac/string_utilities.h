
// string_utilities.h: Utilities for strings for Mac platform

#ifndef COMMON_MAC_STRING_UTILITIES_H__
#define COMMON_MAC_STRING_UTILITIES_H__

#include <CoreFoundation/CoreFoundation.h>

#include <string>

namespace MacStringUtils {

// Convert a CoreFoundation string into a std::string
std::string ConvertToString(CFStringRef str);

// Return the idx'th decimal integer in str, separated by non-decimal-digits
// E.g., str = 10.4.8, idx = 1 -> 4
unsigned int IntegerValueAtIndex(std::string& str, unsigned int idx);

} // namespace MacStringUtils

#endif  // COMMON_MAC_STRING_UTILITIES_H__

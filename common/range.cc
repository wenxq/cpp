
#include "range.h"

#if HAVE_EMMINTRIN_H
    #include <emmintrin.h>  // __v16qi
#endif
#include <iostream>

/**
 * Predicates that can be used with qfind and startsWith
 */
const AsciiCaseSensitive asciiCaseSensitive = AsciiCaseSensitive();
const AsciiCaseInsensitive asciiCaseInsensitive = AsciiCaseInsensitive();

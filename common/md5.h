
#ifndef _COMMON_MD5_H_
#define _COMMON_MD5_H_

#include <stdint.h>

struct MD5Context
{
    uint32_t buf[4];
    uint32_t bits[2];
    uint8_t  in[64];
};

void MD5Init(struct MD5Context* ctx);

void MD5Update(struct MD5Context* ctx, unsigned char const* buf, size_t len);

void MD5Final(unsigned char digest[16], struct MD5Context* ctx);

#endif // _COMMON_MD5_H_


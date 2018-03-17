
#ifndef _FILE_UTILS_H_
#define _FILE_UTILS_H_

#include <assert.h>
#include <stdio.h>
#include <string>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <string>
#include <system_error>

#include "env.h"

inline bool SafeGetEnv(const char* varname, std::string& valstr)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    char*  val;
    size_t sz;
    if (_dupenv_s(&val, &sz, varname) != 0 || !val) return false;
    valstr = val;
    free(val);
#else
    const char* const val = getenv(varname);
    if (!val) return false;
    valstr = val;
#endif
    return true;
}

inline int SafeFOpen(FILE** fp, const char* fname, const char* mode)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
    return fopen_s(fp, fname, mode);
#else
    assert(fp != NULL);
    *fp = fopen(fname, mode);
    // errno only guaranteed to be set on failure
    return ((*fp == NULL) ? errno : 0);
#endif
}

Status CopyFile(Env* env, const std::string& source,
                const std::string& destination, uint64_t size,
                bool use_fsync);

Status CreateFile(Env* env, const std::string& destination,
                const std::string& contents);

#endif  // _FILE_UTILS_H_

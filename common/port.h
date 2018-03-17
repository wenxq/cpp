// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _PORT_H_
#define _PORT_H_

#include <string.h>

// Include the appropriate platform specific file below.  If you are
// porting to a new platform, see "port_example.h" for documentation
// of what the new port_<platform>.h file must provide.
#if defined(WIN32)

#include "windows/port_win.h"

namespace port {

struct dirent
{
    char d_name[_MAX_PATH]; /* filename */
};

struct DIR;

DIR* opendir(const char* name);

dirent* readdir(DIR* dirp);

int closedir(DIR* dirp);

}  // namespace port

using port::dirent;
using port::DIR;
using port::opendir;
using port::readdir;
using port::closedir;

#else

#include <dirent.h>
#include <sys/types.h>
#include "linux/port_posix.h"

#endif  // WIN32

#endif  // _PORT_H_

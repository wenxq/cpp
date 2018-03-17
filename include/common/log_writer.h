// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef _LOG_WRITER_H_
#define _LOG_WRITER_H_

#include <stdint.h>
#include "log_format.h"
#include "slice.h"
#include "status.h"

class WritableFile;

class LogWriter
{
public:
    // Create a writer that will append data to "*dest".
    // "*dest" must be initially empty.
    // "*dest" must remain live while this LogWriter is in use.
    explicit LogWriter(WritableFile* dest);

    // Create a writer that will append data to "*dest".
    // "*dest" must have initial length "dest_length".
    // "*dest" must remain live while this LogWriter is in use.
    LogWriter(WritableFile* dest, uint64_t dest_length);

    ~LogWriter();

    Status AddRecord(const Slice& slice);

private:
    Status EmitPhysicalRecord(log_format::RecordType type, const char* ptr, size_t length);

    // No copying allowed
    LogWriter(const LogWriter&);
    void operator=(const LogWriter&);

private:
    WritableFile* dest_;
    int block_offset_;       // Current offset in block

    // crc32c values for all supported record types.  These are
    // pre-computed to reduce the overhead of computing the crc of the
    // record type stored in the header.
    uint32_t type_crc_[log_format::kMaxRecordType + 1];
};

#endif  // _LOG_WRITER_H_

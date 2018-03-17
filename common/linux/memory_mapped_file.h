
// memory_mapped_file.h: Define the google_breakpad::MemoryMappedFile
// class, which maps a file into memory for read-only access.

#ifndef COMMON_LINUX_MEMORY_MAPPED_FILE_H_
#define COMMON_LINUX_MEMORY_MAPPED_FILE_H_

#include <stddef.h>
#include "basictypes.h"
#include "linux/memory_range.h"

// A utility class for mapping a file into memory for read-only access of
// the file content. Its implementation avoids calling into libc functions
// by directly making system calls for open, close, mmap, and munmap.
class MemoryMappedFile
{
public:
    MemoryMappedFile();

    // Constructor that calls Map() to map a file at |path| into memory.
    // If Map() fails, the object behaves as if it is default constructed.
    MemoryMappedFile(const char* path, size_t offset);

    ~MemoryMappedFile();

    // Maps a file at |path| into memory, which can then be accessed via
    // content() as a MemoryRange object or via data(), and returns true on
    // success. Mapping an empty file will succeed but with data() and size()
    // returning NULL and 0, respectively. An existing mapping is unmapped
    // before a new mapping is created.
    bool Map(const char* path, size_t offset);

    // Unmaps the memory for the mapped file. It's a no-op if no file is
    // mapped.
    void Unmap();

    // Returns a MemoryRange object that covers the memory for the mapped
    // file. The MemoryRange object is empty if no file is mapped.
    const MemoryRange& content() const
    {
        return content_;
    }

    // Returns a pointer to the beginning of the memory for the mapped file.
    // or NULL if no file is mapped or the mapped file is empty.
    const void* data() const
    {
        return content_.data();
    }

    // Returns the size in bytes of the mapped file, or zero if no file
    // is mapped.
    size_t size() const
    {
        return content_.length();
    }

private:
    // Mapped file content as a MemoryRange object.
    MemoryRange content_;

    DISALLOW_COPY_AND_ASSIGN(MemoryMappedFile);
};

#endif  // COMMON_LINUX_MEMORY_MAPPED_FILE_H_

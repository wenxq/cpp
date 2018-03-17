
#include "linux/memory_mapped_file.h"
#include "linux/memory_range.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

MemoryMappedFile::MemoryMappedFile() {}

MemoryMappedFile::MemoryMappedFile(const char* path, size_t offset)
{
    Map(path, offset);
}

MemoryMappedFile::~MemoryMappedFile()
{
    Unmap();
}

bool MemoryMappedFile::Map(const char* path, size_t offset)
{
    Unmap();

    int fd = open(path, O_RDONLY, 0);
    if (fd == -1)
    {
        return false;
    }

#if defined(__x86_64__) || defined(__aarch64__)
    struct stat st;
    if (fstat(fd, &st) == -1 || st.st_size < 0)
    {
#else
    struct stat64 st;
    if (fstat64(fd, &st) == -1 || st.st_size < 0)
    {
#endif
        close(fd);
        return false;
    }

    // Strangely file size can be negative, but we check above that it is not.
    size_t file_len = static_cast<size_t>(st.st_size);
    // If the file does not extend beyond the offset, simply use an empty
    // MemoryRange and return true. Don't bother to call mmap()
    // even though mmap() can handle an empty file on some platforms.
    if (offset >= file_len)
    {
        close(fd);
        return true;
    }

#if defined(__x86_64__) || defined(__aarch64__)
    void* data = mmap(NULL, file_len, PROT_READ, MAP_PRIVATE, fd, offset);
#else
    if ((offset & 4095) != 0)
    {
        // Not page aligned.
        close(fd);
        return false;
    }
    void* data = mmap2(NULL, file_len, PROT_READ, MAP_PRIVATE, fd, offset >> 12);
#endif
    close(fd);
    if (data == MAP_FAILED)
    {
        return false;
    }

    content_.Set(data, file_len - offset);
    return true;
}

void MemoryMappedFile::Unmap()
{
    if (content_.data())
    {
        munmap(const_cast<uint8_t*>(content_.data()), content_.length());
        content_.Set(NULL, 0);
    }
}

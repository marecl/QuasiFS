#pragma once

#ifdef _WIN32

// #error unimplemented

#include <cstdint>
#include "host_io_virtual.h"

class HostIO_Win32
{
public:
    HostIO_Win32() = default;
    ~HostIO_Win32() = default;

    virtual int open(const char *path, int flags);
    virtual int close(int fd);

    virtual ssize_t write(int fd, const void *buf, size_t count);
    virtual ssize_t read(int fd, void *buf, size_t count);
}

#endif
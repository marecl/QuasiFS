#pragma once

#ifdef __linux__

#include <cstdint>
#include "host_io_base.h"

class HostIO_POSIX : HostIO_Base
{
public:
    virtual int open(const char *path, int flags);
    virtual int close(int fd);

    virtual ssize_t write(int fd, const void *buf, size_t count);
    virtual ssize_t read(int fd, void *buf, size_t count);
};
#endif
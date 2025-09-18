#pragma once

#include <sys/types.h>

#include "quasi_errno.h"

class HostIO_Base
{
protected:
    static errno_t _errno;

public:
    static const errno_t geterrno() { return _errno; }

    virtual int open(const char *path, int flags) { return -QUASI_EINVAL; };
    virtual int close(int fd) { return -QUASI_EINVAL; };

    virtual ssize_t write(int fd, const void *buf, size_t count) { return -QUASI_EINVAL; };
    virtual ssize_t read(int fd, void *buf, size_t count) { return -QUASI_EINVAL; };
};
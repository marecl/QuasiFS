#include <cstdio>
#include <string>
#include <iostream>
#include "dev_std.h"

namespace Devices
{

    // stdout
    ssize_t DevStdout::read(off_t offset, void *buf, size_t count)
    {
        return 0;
    }
    ssize_t DevStdout::write(off_t offset, const void *buf, size_t count)
    {
        return fwrite(buf, 1, count, stdout);
    }

    // stdin
    ssize_t DevStdin::read(off_t offset, void *buf, size_t count)
    {
        return fread(buf, 1, count, stdin);
    }

    ssize_t DevStdin::write(off_t offset, const void *buf, size_t count)
    {
        return 0;
    }

    // stderr
    ssize_t DevStderr::read(off_t offset, void *buf, size_t count)
    {
        return 0;
    }

    ssize_t DevStderr::write(off_t offset, const void *buf, size_t count)
    {
        return fwrite(buf, 1, count, stdout);
    }
}
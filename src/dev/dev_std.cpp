#include <cstdio>
#include <string>
#include <iostream>
#include "dev_std.h"

namespace Devices
{

    // stdout
    quasi_ssize_t DevStdout::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        return 0;
    }
    quasi_ssize_t DevStdout::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        return fwrite(buf, 1, count, stdout);
    }

    // stdin
    quasi_ssize_t DevStdin::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        return fread(buf, 1, count, stdin);
    }
    quasi_ssize_t DevStdin::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        return 0;
    }

    // stderr
    quasi_ssize_t DevStderr::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        return 0;
    }
    quasi_ssize_t DevStderr::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        return fwrite(buf, 1, count, stdout);
    }

    // /dev/random
    quasi_ssize_t DevRandom::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        memset(buf, 0xAA, count);
        return count;
    }
    quasi_ssize_t DevRandom::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        return 0;
    }

    // /dev/zero
    quasi_ssize_t DevZero::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        memset(buf, 0, count);
        return count;
    }
    quasi_ssize_t DevZero::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        return 0;
    }

    // /dev/read
    quasi_ssize_t DevNull::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        return 0;
    }
    quasi_ssize_t DevNull::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        return count;
    }
}
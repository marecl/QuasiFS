#pragma once

#include <cstring>

#include "../../quasifs/include/quasifs_inode_regularfile.h"

namespace Devices
{

    class DevStdout : public QuasiFS::RegularFile
    {
    public:
        DevStdout() = default;
        ~DevStdout() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };

    class DevStdin : public QuasiFS::RegularFile
    {
    public:
        DevStdin() = default;
        ~DevStdin() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };

    class DevStderr : public QuasiFS::RegularFile
    {
    public:
        DevStderr() = default;
        ~DevStderr() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };

    class DevRandom : public QuasiFS::RegularFile
    {
    public:
        DevRandom() = default;
        ~DevRandom() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };

    class DevZero : public QuasiFS::RegularFile
    {
    public:
        DevZero() = default;
        ~DevZero() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };

    class DevNull : public QuasiFS::RegularFile
    {
    public:
        DevNull() = default;
        ~DevNull() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };
}
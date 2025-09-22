#pragma once

#include <cstring>

#include "../../quasifs/include/quasifs_inode_regularfile.h"

namespace Devices
{
    using namespace QuasiFS;

    class DevStdout : public QuasiFS::RegularFile
    {
    public:
        DevStdout() = default;
        ~DevStdout() = default;

        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
    };

    class DevStdin : public QuasiFS::RegularFile
    {
    public:
        DevStdin() = default;
        ~DevStdin() = default;

        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
    };

    class DevStderr : public QuasiFS::RegularFile
    {
    public:
        DevStderr() = default;
        ~DevStderr() = default;

        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
    };

    class DevRandom : public QuasiFS::RegularFile
    {
    public:
        DevRandom() = default;
        ~DevRandom() = default;

        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
    };

    class DevZero : public QuasiFS::RegularFile
    {
    public:
        DevZero() = default;
        ~DevZero() = default;

        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
    };

    class DevNull : public QuasiFS::RegularFile
    {
    public:
        DevNull() = default;
        ~DevNull() = default;

        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
    };
}
// INAA License @marecl 2025

#pragma once

#ifdef __linux__

#include "../quasifs/quasi_types.h"
#include "../quasifs/quasi_errno.h"
#include "../quasifs/quasi_sys_fcntl.h"
#include <sys/fcntl.h>
#include "src/host_io_base.h"

namespace HostIODriver
{

    /**
     * This driver bends time and space a bit, because it conforms to POSIX-like returns,
     * i.e. -1 for error, 0 for success
     * However there is no indication that the function executed (because it doesn't have to)
     */

    class HostIO_POSIX : public HostIO_Base
    {

    public:
        //
        // Conversion helpers
        //

        static constexpr int ToPOSIXSeekOrigin(QuasiFS::SeekOrigin origin)
        {
            switch (origin)
            {
            case QuasiFS::SeekOrigin::ORIGIN:
                return SEEK_SET;
            case QuasiFS::SeekOrigin::CURRENT:
                return SEEK_CUR;
            case QuasiFS::SeekOrigin::END:
                return SEEK_END;
            default:
                return -1;
            }
        }

        static constexpr int ToPOSIXOpenFlags(int quasi_flags)
        {
            int flags = 0;
            if (quasi_flags & QUASI_O_RDONLY)
                flags |= O_RDONLY;
            if (quasi_flags & QUASI_O_WRONLY)
                flags |= O_WRONLY;
            if (quasi_flags & QUASI_O_RDWR)
                flags |= O_RDWR;
            if (quasi_flags & QUASI_O_CREAT)
                flags |= O_CREAT;
            if (quasi_flags & QUASI_O_EXCL)
                flags |= O_EXCL;
            if (quasi_flags & QUASI_O_TRUNC)
                flags |= O_TRUNC;
            if (quasi_flags & QUASI_O_APPEND)
                flags |= O_APPEND;
            if (quasi_flags & QUASI_O_NONBLOCK)
                flags |= O_NONBLOCK;
            if (quasi_flags & QUASI_O_SYNC)
                flags |= O_SYNC;
            if (quasi_flags & QUASI_O_FSYNC)
                flags |= O_FSYNC;
            if (quasi_flags & QUASI_O_DIRECTORY)
                flags |= O_DIRECTORY;
            if (quasi_flags & QUASI_O_DIRECT)
                flags |= O_DIRECT;
            if (quasi_flags & QUASI_O_DSYNC)
                flags |= O_DSYNC;

            return flags;
        }

        static constexpr mode_t ToPOSIXOpenMode(quasi_mode_t quasi_mode)
        {
            return quasi_mode;
        }

        static constexpr quasi_errno_t ToQuasiErrno(int errno_val)
        {
            return errno_val;
        }

        //
        // POSIX Functions
        //

        int Open(const fs::path &path, int flags, quasi_mode_t mode = 0755) override;
        int Creat(const fs::path &path, quasi_mode_t mode = 0755) override;
        int Close(const int fd) override;

        int LinkSymbolic(const fs::path &src, const fs::path &dst) override;
        int Link(const fs::path &src, const fs::path &dst) override;
        int Unlink(const fs::path &path) override;
        int Flush(const int fd) override;
        int FSync(const int fd) override;
        int Truncate(const fs::path &path, quasi_size_t size) override;
        int FTruncate(const int fd, quasi_size_t size) override;
        quasi_off_t LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin) override;
        quasi_ssize_t Tell(const int fd) override;
        quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count) override;
        quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset) override;
        quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count) override;
        quasi_ssize_t PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset) override;
        int MKDir(const fs::path &path, quasi_mode_t mode = 0755) override;
        int RMDir(const fs::path &path) override;

        int Stat(const fs::path &path, quasi_stat_t *statbuf) override;
        int FStat(const int fd, quasi_stat_t *statbuf) override;

        int Chmod(const fs::path &path, quasi_mode_t mode) override;
        int FChmod(const int fd, quasi_mode_t mode) override;
    };
}

#endif
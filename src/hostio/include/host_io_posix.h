#pragma once

#ifdef __linux__

#include "host_io_base.h"

namespace HostIODriver
{

    class HostIO_POSIX : HostIO_Base
    {

    public:
        static constexpr int ToSeekOrigin(QuasiFS::SeekOrigin origin)
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

        static constexpr int ToOpenFlags(int quasi_flags)
        {
            return quasi_flags;
        }

        static constexpr mode_t ToOpenMode(quasi_mode_t quasi_mode)
        {
            return quasi_mode;
        }

        //
        // POSIX Functions
        //

        // remember to change mode back to parameter
        int Open(const fs::path &path, int flags, quasi_mode_t mode=0755);
        int Creat(const fs::path &path, quasi_mode_t mode=0755);
        int Close(const int fd);

        int Unlink(const fs::path &path);
        int Flush(const int fd);
        int FSync(const int fd);
        int Truncate(const fs::path &path, quasi_size_t size);
        int FTruncate(const int fd, quasi_size_t size);
        quasi_off_t LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin);
        quasi_ssize_t Tell(const int fd);
        quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count);
        quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset);
        quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count);
        quasi_ssize_t PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset);
        int MKDir(const fs::path &path, quasi_mode_t mode=0755);

        int Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf);
        int FStat(const int fd, QuasiFS::quasi_stat_t *statbuf);
    };
}

#endif
#pragma once

#ifdef __linux__

#include <cstdint>
#include <cstdio>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "host_io_base.h"

namespace HostIODriver
{

    class HostIO_POSIX : HostIO_Base
    {

    public:
        constexpr int ToSeekOrigin(QuasiFS::SeekOrigin origin)
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

        constexpr int ToOpenFlags(int quasi_flags)
        {
            return quasi_flags;
        }

        constexpr int ToOpenMode(int quasi_mode)
        {
            return quasi_mode;
        }

        //
        // POSIX Functions
        //

        // remember to change mode back to parameter
        int Open(const fs::path &path, int flags, mode_t mode = 0755) { return open(path.string().c_str(), flags, mode); };
        int Creat(const fs::path &path, mode_t mode = 0755) { return creat(path.string().c_str(), mode); };
        int Close(const int fd) { return close(fd); };

        // int Unlink(const fs::path &path) { return -1; };
        // int Flush(const int fd) { return -1; };
        // int FSync(const int fd) { return -1; };
        // int FTruncate(const int fd, size_t size) { return -1; };
        // off_t LSeek(const int fd, off_t offset, QuasiFS::SeekOrigin origin) { return -1; };
        // ssize_t Tell(int fd) { return -1; };
        ssize_t Write(int fd, const void *buf, size_t count) { return write(fd, buf, count); };
        ssize_t PWrite(int fd, const void *buf, size_t count, off_t offset) { return pwrite(fd, buf, count, offset); };
        ssize_t Read(int fd, void *buf, size_t count) { return read(fd, buf, count); };
        ssize_t PRead(int fd, void *buf, size_t count, off_t offset) { return pread(fd, buf, count, offset); }
        int MKDir(const fs::path &path, int mode = 0755) { return mkdir(path.string().c_str(), mode); };

        // int Stat(fs::path &path, QuasiFS::quasi_stat_t *stat) { return -1; };
        // int FStat(int fd, QuasiFS::quasi_stat_t *statbuf) { return -1; };
    };
}

#endif
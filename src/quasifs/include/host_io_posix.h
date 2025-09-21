#pragma once

#ifdef __linux__

#include <cstdint>
#include "host_io_base.h"

namespace QuasiFS
{
    namespace HostIO
    {

        class HostIO_POSIX : HostIO_Base
        {

        public:
            constexpr int ToSeekOrigin(SeekOrigin origin)
            {
                switch (origin)
                {
                case SeekOrigin::ORIGIN:
                    return SEEK_SET;
                case SeekOrigin::CURRENT:
                    return SEEK_CUR;
                case SeekOrigin::END:
                    return SEEK_END;
                default:
                    return -1;
                }
            }

            //
            // POSIX Functions
            //
            virtual int Open(const fs::path &path, int flags, int mode) override { return -1; };
            virtual int Close(const int fd) override { return -1; };
            virtual int Unlink(const fs::path &path) override { return -1; };
            virtual int Flush(const int fd) override { return -1; };
            virtual int FSync(const int fd) override { return -1; };
            virtual int FTruncate(const int fd, size_t size) override { return -1; };
            virtual off_t LSeek(const int fd, off_t offset, SeekOrigin origin) override { return -1; };
            virtual ssize_t Tell(int fd) override { return -1; };
            virtual ssize_t Write(int fd, const void *buf, size_t count) override { return -1; };
            virtual ssize_t Read(int fd, void *buf, size_t count) override { return -1; };
            virtual int MKDir(const fs::path &path, int mode) override { return -1; };

            virtual int Stat(fs::path &path, QuasiFS::Stat *stat) override { return -1; };
            virtual int FStat(int fd, QuasiFS::Stat *statbuf) override { return -1; };
        };
    }
}

#endif
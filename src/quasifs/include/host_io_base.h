#pragma once

#include <filesystem>
#include <system_error>

#include <sys/types.h>

#include "quasi_errno.h"
#include "quasifs_types.h"

namespace QuasiFS
{
    namespace HostIO
    {
        namespace fs = std::filesystem;

        /**
         * TODO: throw memory exception in some c++ ports
         * rename, move, copy, remove_all
         */

        class HostIO_Base
        {

        protected:
            static errno_t _errno;

        public:
            HostIO_Base();
            ~HostIO_Base();
            //
            // Internal, OS-specific
            //
            static const errno_t geterrno() { return _errno; }

            //
            // Native wrappers
            //

            virtual int Open(const fs::path &path, int flags, int mode);
            virtual int Close(const int fd);
            virtual int Link(const fs::path &src, const fs::path &dst);
            virtual int Unlink(const fs::path &path);
            virtual int Flush(const int fd);
            virtual int FSync(const int fd);
            virtual int Truncate(const fs::path &path, size_t size);
            virtual int FTruncate(const int fd, size_t size);
            virtual off_t LSeek(const int fd, off_t offset, QuasiFS::SeekOrigin origin);
            virtual ssize_t Tell(int fd);
            virtual ssize_t Write(int fd, const void *buf, size_t count);
            virtual ssize_t PWrite(int fd, const void *buf, size_t count, off_t offset);
            virtual ssize_t Read(int fd, void *buf, size_t count);
            virtual ssize_t PRead(int fd, const void *buf, size_t count, off_t offset);
            virtual int MKDir(const fs::path &path, int mode);
            virtual int RMDir(const fs::path &path);

            virtual int Stat(fs::path &path, QuasiFS::Stat *stat);
            virtual int FStat(int fd, QuasiFS::Stat *statbuf);

            //
            // Derived, complex functions are to be handled by main FS class
            //
        };
    }
}
#pragma once

#include <filesystem>
#include <system_error>

#include <sys/types.h>

#include "../../quasifs/include/quasifs_types.h"
#include "../../quasifs/include/quasi_errno.h"

namespace HostIODriver
{
    namespace fs = std::filesystem;

    /**
     * TODO: throw memory exception in some c++ ports
     * rename, move, copy, remove_all
     */

    class HostIO_Base
    {

    protected:
        errno_t _errno{};

    public:
        HostIO_Base();
        ~HostIO_Base();

        //
        // Internal, OS-specific
        //
        const errno_t geterrno() const { return _errno; }

        //
        // Native wrappers
        //

        int Open(const fs::path &path, int flags, mode_t mode);
        int Creat(const fs::path &path, mode_t mode);
        int Close(const int fd);
        int Link(const fs::path &src, const fs::path &dst);
        int Unlink(const fs::path &path);
        int Flush(const int fd);
        int FSync(const int fd);
        int Truncate(const fs::path &path, size_t size);
        int FTruncate(const int fd, size_t size);
        off_t LSeek(const int fd, off_t offset, QuasiFS::SeekOrigin origin);
        ssize_t Tell(int fd);
        ssize_t Write(int fd, const void *buf, size_t count);
        ssize_t PWrite(int fd, const void *buf, size_t count, off_t offset);
        ssize_t Read(int fd, void *buf, size_t count);
        ssize_t PRead(int fd, void *buf, size_t count, off_t offset);
        int MKDir(const fs::path &path, int mode);
        int RMDir(const fs::path &path);

        int Stat(fs::path &path, QuasiFS::quasi_stat_t *stat);
        int FStat(int fd, QuasiFS::quasi_stat_t *statbuf);

        //
        // Derived, complex functions are to be handled by main FS class
        //
    };
}
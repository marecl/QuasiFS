#pragma once

#include <filesystem>
#include <system_error>

#include <sys/types.h>

#include "../../quasifs/include/quasifs_types.h"
#include "../../quasifs/include/quasi_errno.h"

namespace HostIODriver
{
    namespace fs = std::filesystem;
    using namespace QuasiFS;

    class HostIO_Base
    {

    protected:
        quasi_errno_t _errno{};

    public:
        HostIO_Base();
        ~HostIO_Base();

        //
        // Internal, OS-specific
        //
        const quasi_errno_t geterrno() const { return _errno; }

        //
        // Native wrappers
        //

        int Open(const fs::path &path, int flags, quasi_mode_t mode=0755 );
        int Creat(const fs::path &path, quasi_mode_t mode=0755 );
        int Close(const int fd);
        int Link(const fs::path &src, const fs::path &dst);
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
        int MKDir(const fs::path &path, quasi_mode_t mode =0755);
        int RMDir(const fs::path &path);

        int Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf);
        int FStat(const int fd, QuasiFS::quasi_stat_t *statbuf);

        //
        // Derived, complex functions are to be handled by main FS class
        //
    };
}
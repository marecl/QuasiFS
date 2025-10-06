// INAA License @marecl 2025

#pragma once

#include <filesystem>
#include <system_error>

#include <dirent.h>

#include "../../quasifs/quasi_types.h"
#include "../../quasifs/quasi_sys_stat.h"

namespace HostIODriver
{
    namespace fs = std::filesystem;
    using namespace QuasiFS;
    using namespace QuasiFS::Stat;

    class HostIO_Base
    {

    protected:
    public:
        HostIO_Base();
        ~HostIO_Base();

        //
        // Native wrappers
        //

        virtual int Open(const fs::path &path, int flags, quasi_mode_t mode = 0755);
        virtual int Creat(const fs::path &path, quasi_mode_t mode = 0755);
        virtual int Close(const int fd);

        virtual int LinkSymbolic(const fs::path &src, const fs::path &dst);
        virtual int Link(const fs::path &src, const fs::path &dst);
        virtual int Unlink(const fs::path &path);
        virtual int Flush(const int fd);
        virtual int FSync(const int fd);
        virtual int Truncate(const fs::path &path, quasi_size_t size);
        virtual int FTruncate(const int fd, quasi_size_t size);
        virtual quasi_off_t LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin);
        virtual quasi_ssize_t Tell(const int fd);
        virtual quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count);
        virtual quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset);
        virtual quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count);
        virtual quasi_ssize_t PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset);
        virtual int MKDir(const fs::path &path, quasi_mode_t mode = 0755);
        virtual int RMDir(const fs::path &path);

        virtual int Stat(const fs::path &path, quasi_stat_t *statbuf);
        virtual int FStat(const int fd, quasi_stat_t *statbuf);

        virtual int Chmod(const fs::path &path, quasi_mode_t mode);
        virtual int FChmod(const int fd, quasi_mode_t mode);
        //
        // Derived, complex functions are to be handled by main FS class
        //
    };
}
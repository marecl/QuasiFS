#pragma once

#include <filesystem>
#include <system_error>

#include <sys/types.h>
#include <dirent.h>

#include "../../quasifs/include/quasifs_types.h"
#include "../../quasifs/include/quasi_errno.h"

#include "host_io_base.h"

namespace HostIODriver
{
    namespace fs = std::filesystem;
    using namespace QuasiFS;

    class HostIO_Virtual : public HostIO_Base
    {
    protected:
        Resolved *res;

    public:
        HostIO_Virtual();
        ~HostIO_Virtual();

        //
        // Resolved holds necessary context to perform any FS operation
        // Set context immediately before target operation and clear immediately after
        // to avoid accidental mixups
        //

        void set(Resolved &res)
        {
            this->res = &res;
        }

        void clear(void)
        {
            this->res = nullptr;
        }

        //
        // Native wrappers
        //

        // int Open(const fs::path &path, int flags, quasi_mode_t mode = 0755) override;
        // int Creat(const fs::path &path, quasi_mode_t mode = 0755) override;
        // int Close(const int fd) override;

        // int Link(const fs::path &src, const fs::path &dst) override;
        // int Unlink(const fs::path &path) override;
        // int Flush(const int fd) override;
        // int FSync(const int fd) override;
        // int Truncate(const fs::path &path, quasi_size_t size) override;
        // int FTruncate(const int fd, quasi_size_t size) override;
        // quasi_off_t LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin) override;
        // quasi_ssize_t Tell(const int fd) override;
        // quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count) override;
        // quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset) override;
        // quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count) override;
        // quasi_ssize_t PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset) override;
        int MKDir(const fs::path &path, quasi_mode_t mode = 0755) override;
        // int RMDir(const fs::path &path) override;

        // int Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf) override;
        // int FStat(const int fd, QuasiFS::quasi_stat_t *statbuf) override;

        //
        // Derived, complex functions are to be handled by main FS class
        //
    };
}
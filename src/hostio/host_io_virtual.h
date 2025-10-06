// INAA License @marecl 2025

#pragma once

#include <filesystem>
#include <system_error>

#include <dirent.h>

#include "../quasifs/quasi_types.h"
#include "../quasifs/quasi_errno.h"

#include "src/host_io_base.h"

namespace HostIODriver
{
    class HostIO_Virtual : public HostIO_Base
    {
    protected:
        Resolved *res{nullptr};
        fd_handle_ptr handle{nullptr};
        bool host_bound{false};

    public:
        HostIO_Virtual();
        ~HostIO_Virtual();

        //
        // Resolved holds necessary context to perform any FS operation
        // Set context immediately before target operation and clear immediately after
        // to avoid accidental mixups
        // Path is more of a... suggestion where we might be.
        //

        void SetCtx(Resolved *res, bool host_bound, fd_handle_ptr handle)
        {
            this->res = res;
            this->host_bound = host_bound;
            this->handle = handle;
        }

        void ClearCtx(void)
        {
            this->res = nullptr;
            this->handle = nullptr;
            this->host_bound = false;
        }

        //
        // Native wrappers
        //

        /**
         * WARNING
         * This function doesn't return numeric fd
         * The only outputs are 0 and -errno!
         */
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

        //
        // Derived, complex functions are to be handled by main FS class
        //
    };
}
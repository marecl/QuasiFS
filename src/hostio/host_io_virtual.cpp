#include <filesystem>

#include <sys/types.h>

#include "../../quasifs/include/quasi_errno.h"
#include "../../quasifs/include/quasifs_types.h"
#include "../../quasifs/include/quasifs_inode_directory.h"
#include "../../quasifs/include/quasifs_inode_regularfile.h"
#include "../../quasifs/include/quasifs_inode_symlink.h"
#include "../../quasifs/include/quasifs_partition.h"

#include "host_io_base.h"
#include "host_io_virtual.h"

namespace HostIODriver
{

    HostIO_Virtual::HostIO_Virtual() = default;
    HostIO_Virtual::~HostIO_Virtual() = default;

    // int HostIO_Virtual::Open(const fs::path &path, int flags, quasi_mode_t mode)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Creat(const fs::path &path, quasi_mode_t mode)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Close(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Link(const fs::path &src, const fs::path &dst)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Unlink(const fs::path &path)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Flush(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::FSync(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Truncate(const fs::path &path, quasi_size_t size)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::FTruncate(const int fd, quasi_size_t size)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_off_t HostIO_Virtual::LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::Tell(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::Write(const int fd, const void *buf, quasi_size_t count)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::Read(const int fd, void *buf, quasi_size_t count)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    int HostIO_Virtual::MKDir(const fs::path &path, quasi_mode_t mode)
    {
        if (nullptr == this->res)
            return -1;

        dir_ptr new_dir = Directory::Create();
        return this->res->mountpoint->mkdir(this->res->parent, this->res->leaf, new_dir);
    }

    // int HostIO_Virtual::RMDir(const fs::path &path)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::FStat(const int fd, QuasiFS::quasi_stat_t *statbuf)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

}

#include <cstdint>
#include <cstdio>
#include <dirent.h>

#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "../../quasifs/include/quasi_errno.h"
#include "../../quasifs/include/quasifs_types.h"
#include "../../quasifs/include/quasifs_inode_directory.h"
#include "../../quasifs/include/quasifs_inode_regularfile.h"
#include "../../quasifs/include/quasifs_inode_symlink.h"
#include "../../quasifs/include/quasifs_partition.h"

#include "host_io_base.h"
#include "host_io_posix.h"

namespace HostIODriver
{

    int HostIO_POSIX::Open(const fs::path &path, int flags, quasi_mode_t mode)
    {
        errno = 0;
        int status = open(path.c_str(), ToPOSIXOpenFlags(flags), ToPOSIXOpenMode(mode));
        return status >= 0 ? status : -errno;
    }

    int HostIO_POSIX::Creat(const fs::path &path, quasi_mode_t mode)
    {
        errno = 0;
        int status = creat(path.c_str(), ToPOSIXOpenMode(mode));
        return status >= 0 ? status : -errno;
    }

    int HostIO_POSIX::Close(const int fd)
    {
        errno = 0;
        int status = close(fd);
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::LinkSymbolic(const fs::path &src, const fs::path &dst)
    {
        errno = 0;
        int status = symlink(src.c_str(), dst.c_str());
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::Link(const fs::path &src, const fs::path &dst)
    {
        errno = 0;
        int status = link(src.c_str(), dst.c_str());
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::Unlink(const fs::path &path)
    {
        errno = 0;
        int status = unlink(path.c_str());
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::Flush(const int fd)
    {
        errno = 0;
        return 0;
    }

    int HostIO_POSIX::FSync(const int fd)
    {
        errno = 0;
        int status = fsync(fd);
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::Truncate(const fs::path &path, quasi_size_t size)
    {
        errno = 0;
        int status = truncate(path.c_str(), size);
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::FTruncate(const int fd, quasi_size_t size)
    {
        errno = 0;
        int status = ftruncate(fd, size);
        return status == 0 ? status : -errno;
    }

    quasi_off_t HostIO_POSIX::LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin)
    {
        errno = 0;
        int status = lseek(fd, offset, ToPOSIXSeekOrigin(origin));
        return status == 0 ? status : -errno;
    }

    quasi_ssize_t HostIO_POSIX::Tell(const int fd)
    {
        return LSeek(fd, 0, SeekOrigin::CURRENT);
    }

    quasi_ssize_t HostIO_POSIX::Write(const int fd, const void *buf, quasi_size_t count)
    {
        errno = 0;
        int status = write(fd, buf, count);
        return status >= 0 ? status : -errno;
    }

    quasi_ssize_t HostIO_POSIX::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    {
        errno = 0;
        int status = pwrite(fd, buf, count, offset);
        return status >= 0 ? status : -errno;
    }

    quasi_ssize_t HostIO_POSIX::Read(const int fd, void *buf, quasi_size_t count)
    {
        errno = 0;
        int status = read(fd, buf, count);
        return status >= 0 ? status : -errno;
    }

    quasi_ssize_t HostIO_POSIX::PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset)
    {
        errno = 0;
        int status = pread(fd, buf, count, offset);
        return status >= 0 ? status : -errno;
    }

    int HostIO_POSIX::MKDir(const fs::path &path, quasi_mode_t mode)
    {
        errno = 0;
        int status = mkdir(path.c_str(), mode);
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::RMDir(const fs::path &path)
    {
        errno = 0;
        int status = rmdir(path.c_str());
        return status == 0 ? status : -errno;
    }

    int HostIO_POSIX::Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf)
    {
        errno = 0;

        struct stat st;

        if (int stat_status = stat(path.c_str(), &st); stat_status != 0)
            return stat_status;

        // handled by QFS
        // statbuf->st_dev = st.st_dev;
        // statbuf->st_ino = st.st_ino;
        // statbuf->st_nlink = st.st_nlink;

        statbuf->st_mode = st.st_mode;
        statbuf->st_size = st.st_size;
        statbuf->st_blksize = st.st_blksize;
        statbuf->st_blocks = st.st_blocks;
        statbuf->st_atim = st.st_atim;
        statbuf->st_mtim = st.st_mtim;
        statbuf->st_ctim = st.st_ctim;

        return 0;
    }

    int HostIO_POSIX::FStat(const int fd, QuasiFS::quasi_stat_t *statbuf)
    {
        errno = 0;

        struct stat st;

        if (int fstat_status = fstat(fd, &st); fstat_status != 0)
            return fstat_status;

        // handled by QFS
        // statbuf->st_dev = st.st_dev;
        // statbuf->st_ino = st.st_ino;
        // statbuf->st_nlink = st.st_nlink;

        statbuf->st_mode = st.st_mode;
        statbuf->st_size = st.st_size;
        statbuf->st_blksize = st.st_blksize;
        statbuf->st_blocks = st.st_blocks;
        statbuf->st_atim = st.st_atim;
        statbuf->st_mtim = st.st_mtim;
        statbuf->st_ctim = st.st_ctim;

        return 0;
    }
}
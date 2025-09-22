#include <cstdint>
#include <cstdio>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "host_io_base.h"
#include "host_io_posix.h"

namespace HostIODriver
{

    int HostIO_POSIX::Open(const fs::path &path, int flags, quasi_mode_t mode)
    {
        errno = this->_errno = 0;
        int status = open(path.string().c_str(), ToOpenFlags(flags), ToOpenMode(mode));
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::Creat(const fs::path &path, quasi_mode_t mode)
    {
        errno = this->_errno = 0;
        int status = creat(path.string().c_str(), ToOpenMode(mode));
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::Close(const int fd)
    {
        errno = this->_errno = 0;
        int status = close(fd);
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::Unlink(const fs::path &path)
    {
        errno = this->_errno = 0;
        int status = unlink(path.string().c_str());
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::Flush(const int fd)
    {
        // no equivalent in POSIX
        _errno = 0;
        return 0;
    }

    int HostIO_POSIX::FSync(const int fd)
    {
        errno = this->_errno = 0;
        int status = fsync(fd);
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::Truncate(const fs::path &path, quasi_size_t size)
    {
        errno = this->_errno = 0;
        int status = truncate(path.string().c_str(), size);
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::FTruncate(const int fd, quasi_size_t size)
    {
        errno = this->_errno = 0;
        int status = ftruncate(fd, size);
        this->_errno = errno;
        return status;
    }

    quasi_off_t HostIO_POSIX::LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin)
    {
        errno = this->_errno = 0;
        int status = lseek(fd, offset, ToSeekOrigin(origin));
        this->_errno = errno;
        return status;
    }

    quasi_ssize_t HostIO_POSIX::Tell(const int fd)
    {
        errno = this->_errno = 0;
        int status = lseek(fd, 0, SEEK_CUR);
        this->_errno = errno;
        return status;
    }

    quasi_ssize_t HostIO_POSIX::Write(const int fd, const void *buf, quasi_size_t count)
    {
        errno = this->_errno = 0;
        int status = write(fd, buf, count);
        this->_errno = errno;
        return status;
    }

    quasi_ssize_t HostIO_POSIX::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    {
        errno = this->_errno = 0;
        int status = pwrite(fd, buf, count, offset);
        this->_errno = errno;
        return status;
    }

    quasi_ssize_t HostIO_POSIX::Read(const int fd, void *buf, quasi_size_t count)
    {
        errno = this->_errno = 0;
        int status = read(fd, buf, count);
        this->_errno = errno;
        return status;
    }

    quasi_ssize_t HostIO_POSIX::PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset)
    {
        errno = this->_errno = 0;
        int status = pread(fd, buf, count, offset);
        this->_errno = errno;

        return status;
    }

    int HostIO_POSIX::MKDir(const fs::path &path, quasi_mode_t mode)
    {
        errno = this->_errno = 0;
        int status = mkdir(path.string().c_str(), mode);
        this->_errno = errno;
        return status;
    }

    int HostIO_POSIX::Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf)
    {
        struct stat st;

        errno = this->_errno = 0;
        int status = stat(path.string().c_str(), &st);
        _errno = errno;

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

        return status;
    }

    int HostIO_POSIX::FStat(const int fd, QuasiFS::quasi_stat_t *statbuf)
    {
        struct stat st;

        errno = this->_errno = 0;
        int status = fstat(fd, &st);
        _errno = errno;

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

        return status;
    }
}
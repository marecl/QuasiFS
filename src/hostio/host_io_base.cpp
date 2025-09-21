#include <filesystem>

#include <sys/types.h>

#include "host_io_base.h"

namespace HostIODriver
{

    HostIO_Base::HostIO_Base() = default;
    HostIO_Base::~HostIO_Base() = default;

    int HostIO_Base::Open(const fs::path &path, int flags, mode_t mode = 755) { return -1; };
    int HostIO_Base::Creat(const fs::path &path, mode_t mode = 755) { return -1; };
    int HostIO_Base::Close(const int fd) { return -1; };
    int HostIO_Base::Link(const fs::path &src, const fs::path &dst) { return -1; };
    int HostIO_Base::Unlink(const fs::path &path) { return -1; };
    int HostIO_Base::Flush(const int fd) { return -1; };
    int HostIO_Base::FSync(const int fd) { return -1; };
    int HostIO_Base::Truncate(const fs::path &path, size_t size) { return -1; };
    int HostIO_Base::FTruncate(const int fd, size_t size) { return -1; };
    off_t HostIO_Base::LSeek(const int fd, off_t offset, QuasiFS::SeekOrigin origin) { return -1; };
    ssize_t HostIO_Base::Tell(int fd) { return -1; };
    ssize_t HostIO_Base::Write(int fd, const void *buf, size_t count) { return -1; };
    ssize_t HostIO_Base::PWrite(int fd, const void *buf, size_t count, off_t offset) { return -1; };
    ssize_t HostIO_Base::Read(int fd, void *buf, size_t count) { return -1; };
    ssize_t HostIO_Base::PRead(int fd, void *buf, size_t count, off_t offset) { return -1; };
    int HostIO_Base::MKDir(const fs::path &path, int mode = 755) { return -1; };
    int HostIO_Base::RMDir(const fs::path &path) { return -1; };
    int HostIO_Base::Stat(fs::path &path, QuasiFS::quasi_stat_t *stat) { return -1; };
    int HostIO_Base::FStat(int fd, QuasiFS::quasi_stat_t *statbuf) { return -1; };

}
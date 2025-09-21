#include <filesystem>

#include <sys/types.h>

#include "quasi_errno.h"
#include "host_io_base.h"

namespace QuasiFS
{
    namespace HostIO
    {
        HostIO_Base::HostIO_Base() = default;
        HostIO_Base::~HostIO_Base() = default;
        int HostIO_Base::Open(const fs::path &path, int flags, int mode) { return -QUASI_EINVAL; };
        int HostIO_Base::Close(const int fd) { return -QUASI_EINVAL; };
        int HostIO_Base::Link(const fs::path &src, const fs::path &dst) { return -QUASI_EINVAL; };
        int HostIO_Base::Unlink(const fs::path &path) { return -QUASI_EINVAL; };
        int HostIO_Base::Flush(const int fd) { return -QUASI_EINVAL; };
        int HostIO_Base::FSync(const int fd) { return -QUASI_EINVAL; };
        int HostIO_Base::Truncate(const fs::path &path, size_t size) { return -QUASI_EINVAL; };
        int HostIO_Base::FTruncate(const int fd, size_t size) { return -QUASI_EINVAL; };
        off_t HostIO_Base::LSeek(const int fd, off_t offset, SeekOrigin origin) { return -QUASI_EINVAL; };
        ssize_t HostIO_Base::Tell(int fd) { return -QUASI_EINVAL; };
        ssize_t HostIO_Base::Write(int fd, const void *buf, size_t count) { return -QUASI_EINVAL; };
        ssize_t HostIO_Base::PWrite(int fd, const void *buf, size_t count, off_t offset) { return -QUASI_EINVAL; };
        ssize_t HostIO_Base::Read(int fd, void *buf, size_t count) { return -QUASI_EINVAL; };
        ssize_t HostIO_Base::PRead(int fd, const void *buf, size_t count, off_t offset) { return -QUASI_EINVAL; };
        int HostIO_Base::MKDir(const fs::path &path, int mode) { return -QUASI_EINVAL; };
        int HostIO_Base::RMDir(const fs::path &path) { return -QUASI_EINVAL; };
        int HostIO_Base::Stat(fs::path &path, QuasiFS::Stat *stat) { return -QUASI_EINVAL; };
        int HostIO_Base::FStat(int fd, QuasiFS::Stat *statbuf) { return -QUASI_EINVAL; };

    }
}
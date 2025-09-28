#include <filesystem>

#include <sys/types.h>

#include "../../log.h"
#include "host_io_base.h"

#define STUB()                              \
    LogError("Stub called in HostIO_Base"); \
    return -1;

namespace HostIODriver
{

    HostIO_Base::HostIO_Base() = default;
    HostIO_Base::~HostIO_Base() = default;

    int HostIO_Base::Open(const fs::path &path, int flags, quasi_mode_t mode) { STUB(); }
    int HostIO_Base::Creat(const fs::path &path, quasi_mode_t mode) { STUB(); }
    int HostIO_Base::Close(const int fd) { STUB(); }
    int HostIO_Base::LinkSymbolic(const fs::path &src, const fs::path &dst) { STUB(); }
    int HostIO_Base::Link(const fs::path &src, const fs::path &dst) { STUB(); }
    int HostIO_Base::Unlink(const fs::path &path) { STUB(); }
    int HostIO_Base::Flush(const int fd) { STUB(); }
    int HostIO_Base::FSync(const int fd) { STUB(); }
    int HostIO_Base::Truncate(const fs::path &path, quasi_size_t size) { STUB(); }
    int HostIO_Base::FTruncate(const int fd, quasi_size_t size) { STUB(); }
    quasi_off_t HostIO_Base::LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin) { STUB(); }
    quasi_ssize_t HostIO_Base::Tell(const int fd) { STUB(); }
    quasi_ssize_t HostIO_Base::Write(const int fd, const void *buf, quasi_size_t count) { STUB(); }
    quasi_ssize_t HostIO_Base::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset) { STUB(); }
    quasi_ssize_t HostIO_Base::Read(const int fd, void *buf, quasi_size_t count) { STUB(); }
    quasi_ssize_t HostIO_Base::PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset) { STUB(); }
    int HostIO_Base::MKDir(const fs::path &path, quasi_mode_t mode) { STUB(); }
    int HostIO_Base::RMDir(const fs::path &path) { STUB(); }
    int HostIO_Base::Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf) { STUB(); }
    int HostIO_Base::FStat(const int fd, QuasiFS::quasi_stat_t *statbuf) { STUB(); }

}
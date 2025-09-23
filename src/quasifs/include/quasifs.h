#pragma once

#include <unordered_map>
#include <sys/fcntl.h>

#include "../../hostio/include/host_io.h"
#include "quasifs_types.h"

/**
 * Wrapper class
 * Basically a Partition object with a bit of extra functionality and single superblock
 */

namespace QuasiFS
{

    // Very small QFS manager: path resolution, mount, create/unlink
    class QFS : HostIOBase
    {
        // root partition
        partition_ptr rootfs;
        // root directory of the root partition ("/")
        dir_ptr root;

        // partition mappings
        std::unordered_map<uint64_t, partition_ptr> block_devices{};
        // open file descriptors
        std::vector<fd_handle_ptr> open_fd;

        HostIO hio_driver{};
        HostVIO vio_driver{};

    public:
        QFS(const fs::path &host_path = "");
        ~QFS() = default;

        //
        // QFS methods
        //
        int SyncHost(void);
        void SyncHostImpl(partition_ptr &part, const fs::path &dir, std::string prefix = "");
        // Return root directory
        dir_ptr GetRoot() { return this->root; }
        // Return root partition
        partition_ptr GetRootFS() { return this->rootfs; }
        // Resolve path. Output is saved to r, returns status code
        int Resolve(fs::path path, Resolved &r);

        // mount fs at path (target must exist and be directory)
        int Mount(const fs::path &path, partition_ptr fs);
        // mount fs at path (target must exist and be directory)
        int Unmount(const fs::path &path);

        //
        // Inherited from HostIOBase
        // Raw file operations
        //
        int Open(const fs::path &path, int flags, quasi_mode_t mode = 0755);
        int Creat(const fs::path &path, quasi_mode_t mode = 0755);
        int Close(const int fd);
        int LinkSymbolic(const fs::path &src, const fs::path &dst);
        int Link(const fs::path &src, const fs::path &dst);
        int Unlink(const fs::path &path);
        int Flush(const int fd);
        int FSync(const int fd);
        int Truncate(const fs::path &path, quasi_size_t size);
        int FTruncate(const int fd, quasi_size_t size);
        quasi_off_t LSeek(const int fd, quasi_off_t offset, SeekOrigin origin);
        quasi_ssize_t Tell(const int fd);
        quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count);
        quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset);
        quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count);
        quasi_ssize_t PRead(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset);
        int MKDir(const fs::path &path, quasi_mode_t mode = 0755);
        int RMDir(const fs::path &path);

        int Stat(const fs::path &path, quasi_stat_t *stat);
        int FStat(const int fd, quasi_stat_t *statbuf);

        //
        // Complex file operations, QFS specific
        //

        // Create empty file in path
        int Touch(const fs::path &path);
        // Create empty file with name in path
        int Touch(const fs::path &path, const std::string &name);
        // Force-insert child at path
        int Touch(const fs::path &path, const std::string &name, file_ptr child);
        // Create directory with name in path
        int MKDir(const fs::path &path, const std::string &name);
        // Force-insert child at path
        int MKDir(const fs::path &path, const std::string &name, dir_ptr child);

        // Remove link from [where]
        // int Unlink(const std::string &where);

        quasi_ssize_t getdirectorysize(const fs::path &path) { return -QUASI_EINVAL; };
        int IsOpen(const int fd) noexcept { return -QUASI_EINVAL; };
        int SetSize(const int fd, uint64_t size) noexcept { return -QUASI_EINVAL; };
        quasi_ssize_t GetSize(const int fd) noexcept { return -QUASI_EINVAL; };
        // Not a port, used by 2-3 functions that *never* check for errors
        quasi_ssize_t GetDirectorySize(const fs::path &path) noexcept { return -QUASI_EINVAL; };
        // Not really necessary but handy
        bool Touch(const fs::path &path, int mode) noexcept { return -QUASI_EINVAL; };

        // C++ ports with both except/noexcept
        int64_t GetSize(const fs::path &path) { return -QUASI_EINVAL; };
        int64_t GetSize(const fs::path &path, std::error_code &ec) noexcept { return -QUASI_EINVAL; };
        bool Exists(const fs::path &path) { return -QUASI_EINVAL; };
        bool Exists(const fs::path &path, std::error_code &ec) noexcept { return -QUASI_EINVAL; };
        bool IsDirectory(const fs::path &path) { return -QUASI_EINVAL; };
        bool IsDirectory(const fs::path &path, std::error_code &ec) noexcept { return -QUASI_EINVAL; };
        fs::path AbsolutePath(const fs::path &path) { return ""; };
        fs::path AbsolutePath(const fs::path &path, std::error_code &ec) noexcept { return ""; };
        bool Remove(const fs::path &path) { return -QUASI_EINVAL; };
        bool Remove(const fs::path &path, std::error_code &ec) noexcept { return -QUASI_EINVAL; };
        uint64_t RemoveAll(const fs::path &path) = delete;
        uint64_t RemoveAll(const fs::path &path, std::error_code &ec) noexcept = delete;
        uint64_t CurrentPath(const fs::path &path) = delete;
        uint64_t CurrentPath(const fs::path &path, std::error_code &ec) noexcept = delete;

        bool Copy(const fs::path &from, const fs::path &to) = delete;
        bool Copy(const fs::path &from, const fs::path &to, std::error_code &ec) noexcept = delete;
        bool Copy(const fs::path &from, const fs::path &to, std::filesystem::copy_options options) = delete;
        bool Copy(const fs::path &from, const fs::path &to, std::filesystem::copy_options options,
                  std::error_code &ec) noexcept = delete;

        // 0777 to mimic default C++ mode (std::filesystem::perms::all)
        bool CreateDirectory(const fs::path &path, int mode = 0777) { return -QUASI_EINVAL; };
        bool CreateDirectory(const fs::path &path, std::error_code &ec, int mode = 0777) noexcept { return -QUASI_EINVAL; };
        bool CreateDirectory(const fs::path &path, const fs::path &existing_path, int mode = 0777) = delete;
        bool CreateDirectory(const fs::path &path, const fs::path &existing_path, std::error_code &ec,
                             int mode = 0777) noexcept = delete;
        bool CreateDirectories(const fs::path &path, int mode = 0777) { return -QUASI_EINVAL; };
        bool CreateDirectories(const fs::path &path, std::error_code &ec, int mode = 0777) noexcept { return -QUASI_EINVAL; };

    private:
        // Get next available fd slot
        int GetFreeHandleNo();
        // partition by blkdev
        partition_ptr GetPartitionByBlockdev(uint64_t blkid);
    };

};
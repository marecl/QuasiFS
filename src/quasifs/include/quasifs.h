#pragma once

#include <unordered_map>
#include <sys/fcntl.h>

#include "../../hostio/include/host_io.h"
#include "../../hostio/include/host_io.h"
#include "quasifs_inode.h"
#include "quasifs_partition.h"

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
        // host-bound files. user should *never* be able to access real file structure
        std::unordered_map<inode_ptr, fs::path> host_files{};

        HostIO driver{};

    public:
        QFS();
        ~QFS() = default;

        //
        // QFS methods
        //

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

        int MapHost(const fs::path &local, const fs::path &host);

        //
        // Inherited from HostIOBase
        // Raw file operations
        //
        int Open(const fs::path &path, int flags, mode_t mode =0755);
        int Creat(const fs::path &path, mode_t mode = 0755);

        int Close(const int fd);
        int LinkSymbolic(const fs::path &src, const fs::path &dst);
        int Link(const fs::path &src, const fs::path &dst);
        int Unlink(const fs::path &path);
        int Flush(const int fd);
        int FSync(const int fd);
        int Truncate(const fs::path &path, size_t size);
        int FTruncate(const int fd, size_t size);
        off_t LSeek(const int fd, off_t offset, SeekOrigin origin);
        ssize_t Tell(int fd);
        ssize_t Write(int fd, const void *buf, size_t count);
        ssize_t PWrite(int fd, const void *buf, size_t count, off_t offset);
        ssize_t Read(int fd, void *buf, size_t count);
        ssize_t PRead(int fd, const void *buf, size_t count, off_t offset);
        int MKDir(const fs::path &path, int mode = 0755);
        int RMDir(const fs::path &path);

        int Stat(fs::path &path, quasi_stat_t *stat);
        int FStat(int fd, quasi_stat_t *statbuf);

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

        ssize_t getdirectorysize(const fs::path &path) { return -QUASI_EINVAL; };
        int IsOpen(const int fd) noexcept { return -QUASI_EINVAL; };
        int SetSize(const int fd, uint64_t size) noexcept { return -QUASI_EINVAL; };
        ssize_t GetSize(const int fd) noexcept { return -QUASI_EINVAL; };
        // Not a port, used by 2-3 functions that *never* check for errors
        ssize_t GetDirectorySize(const fs::path &path) noexcept { return -QUASI_EINVAL; };
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

        fs::path GetHostPathByPath(const fs::path &path);
        fs::path GetHostPathByInode(inode_ptr node);
    };

};
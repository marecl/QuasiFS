#pragma once

#include <unordered_map>
#include <sys/fcntl.h>

#include "quasifs_inode.h"
#include "quasifs_partition.h"

/**
 * Wrapper class
 * Basically a Partition object with a bit of extra functionality and single superblock
 */

namespace QuasiFS
{

    // Very small QFS manager: path resolution, mount, create/unlink
    class QFS
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
        int resolve(fs::path path, Resolved &r);

        // mount fs at path (target must exist and be directory)
        int mount(const fs::path &path, partition_ptr fs);
        // mount fs at path (target must exist and be directory)
        int unmount(const fs::path &path);

        //
        // File manip methods
        // If the file is host-bound, op is delegated to HostIO and synchronized with QFS
        //

        int open(fs::path path, int flags);
        int close(int fd);
        ssize_t write(int fd, const void *buf, size_t count);
        ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
        ssize_t read(int fd, void *buf, size_t count);
        ssize_t pread(int fd, void *buf, size_t count, off_t offset);

        int truncate(const fs::path &path, off_t length);
        int ftruncate(int fd, off_t length);

        // Create empty file in path
        int touch(const fs::path &path);
        // Create empty file with name in path
        int touch(const fs::path &path, const std::string &name);
        // Force-insert child at path
        int touch(const fs::path &path, const std::string &name, file_ptr child);
        // Create directory in path
        int mkdir(const fs::path &path);
        // Create directory with name in path
        int mkdir(const fs::path &path, const std::string &name);
        // Force-insert child at path
        int mkdir(const fs::path &path, const std::string &name, dir_ptr child);
        // Remove directory
        int rmdir(const fs::path &path);

        // Link [what] at [where]
        int link(const fs::path what, const fs::path where);
        // Remove link from [where]
        int unlink(const std::string &where);
        int symlink(const fs::path what, const fs::path where);

    private:
        // Get next available fd slot
        int GetFreeHandleNo();
        // partition by blkdev
        partition_ptr GetPartitionByBlockdev(uint64_t blkid);

        fs::path GetHostPathByPath(const fs::path &path);
        fs::path GetHostPathByInode(inode_ptr node);
    };

};
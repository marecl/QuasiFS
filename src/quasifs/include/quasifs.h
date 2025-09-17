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
        partition_ptr rootfs;
        dir_ptr root;
        // fileno of a directory is a mountpoint
        // std::vector<int,File*> open_handles;

        std::unordered_map<uint64_t, partition_ptr> block_devices{};

        std::vector<fd_handle_ptr> open_fd;

    public:
        QFS();
        ~QFS() = default;

        inode_ptr GetRoot() { return std::static_pointer_cast<Inode>(this->root); }
        partition_ptr GetRootFS() { return std::static_pointer_cast<Partition>(this->rootfs); }

        int resolve(fs::path path, Resolved &r);

        int GetFreeHandleNo();

        int open(fs::path path, int flags);
        int close(int fd);
        ssize_t write(int fd, const void *buf, size_t count);
        ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset);
        ssize_t read(int fd, void *buf, size_t count);
        ssize_t pread(int fd, void *buf, size_t count, off_t offset);

        int truncate(const fs::path &path, off_t length);
        int ftruncate(int fd, off_t length);

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int touch(const fs::path &path);
        int touch(const fs::path &path, const std::string &name);
        // force-insert child at path
        int touch(const fs::path &path, const std::string &name, file_ptr child);

        int mkdir(const fs::path &path);
        int mkdir(const fs::path &path, const std::string &name);
        int mkdir(const fs::path &path, const std::string &name, dir_ptr child);

        int rmdir(const fs::path &path);

        // what is linked where
        int symlink(const fs::path what, const fs::path where);
        int link(const fs::path what, const fs::path where);
        int unlink(const std::string &path);
        // mount fs at path (target must exist and be directory)
        int mount(const fs::path &path, partition_ptr fs);
        // mount fs at path (target must exist and be directory)
        int unmount(const fs::path &path);
    };

};
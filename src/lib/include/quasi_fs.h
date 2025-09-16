#pragma once

#include <unordered_map>

#include "quasi_partition.h"

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
        // symlinks. fictional symlink location, symlink containing target
        std::unordered_map<fs::path, symlink_ptr> symlink_table{};

    public:
        QFS();
        ~QFS() = default;

        inode_ptr GetRoot() { return std::static_pointer_cast<Inode>(this->root); }
        partition_ptr GetRootFS() { return std::static_pointer_cast<Partition>(this->rootfs); }

        Resolved resolve(const fs::path &path);

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int touch(const fs::path &path);
        // what is linked where
        int symlink(const fs::path what, const fs::path where);
        int mkdir(const fs::path &path);
        int rmdir(const fs::path &path);

        int link(const fs::path what, const fs::path where);
        int unlink(const std::string &path);
        // mount fs at path (target must exist and be directory)
        int mount(const fs::path &target_path, partition_ptr fs);
        // mount fs at path (target must exist and be directory)
        int unmount(const fs::path &target_path);
    };

};
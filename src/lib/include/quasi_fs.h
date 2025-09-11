#pragma once

#include <unordered_map>


#include "quasi_types.h"
#include "quasi_partition.h"

namespace QuasiFS
{
    // Very small QFS manager: path resolution, mount, create/unlink
    class QFS
    {
        partition_ptr rootfs;
        dir_ptr root;
        // fileno of a directory is a mountpoint
        std::unordered_map<fileno_t, partition_ptr> fs_table{};
        // std::vector<int,File*> open_handles;

    public:
        QFS();
        ~QFS() = default;

        inode_ptr GetRoot() { return std::static_pointer_cast<Inode>(this->root); }

        Resolved resolve(const fs::path &path);

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int touch(const fs::path &path);
        int mkdir(const fs::path &path);

        int unlink(const std::string &path);
        // mount fs at path (target must exist and be directory)
        int mount(const fs::path &target_path, partition_ptr fs);
        // mount fs at path (target must exist and be directory)
        int unmount(const fs::path &target_path);
    };

};
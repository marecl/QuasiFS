#pragma once

#include <chrono>
#include <filesystem>
#include <sys/types.h>

namespace QuasiFS
{

    namespace fs = std::filesystem;

    using fileno_t = int64_t;
    using time_point = std::chrono::system_clock::time_point;
    using blkid_t = uint64_t;

    // Forward
    class Inode;
    using inode_ptr = std::shared_ptr<Inode>;
    class Partition;
    using partition_ptr = std::shared_ptr<Partition>;
    class RegularFile;
    using file_ptr = std::shared_ptr<RegularFile>;
    class Symlink;
    using symlink_ptr = std::shared_ptr<Symlink>;
    class Directory;
    using dir_ptr = std::shared_ptr<Directory>;

    struct Stat
    {
        uint64_t size = 0;
        uint64_t blocks = 0;
        uint32_t mode = 0; // posix mode bits
        uint64_t ino = 0;
        uint32_t nlink = 0;
        uint64_t dev = 0;
        time_point atime, mtime, ctime;
    };

    // resolve path into (parent_dir, leaf_name, inode)
    struct Resolved
    {
        partition_ptr mountpoint{};
        dir_ptr parent{};
        inode_ptr node{}; // nullptr if doesn't exist
        std::string leaf{};
    };
}
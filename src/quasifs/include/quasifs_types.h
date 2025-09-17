#pragma once

#include <chrono>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

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

    using Stat = struct stat;

    // resolve path into (parent_dir, leaf_name, inode)
    struct Resolved
    {
        partition_ptr mountpoint{};
        dir_ptr parent{};
        inode_ptr node{}; // nullptr if doesn't exist
        std::string leaf{};
    };

    typedef struct File File;
    using fd_handle_ptr = std::shared_ptr<File>;

    struct File
    {
        File() = default;
        ~File() = default;
        fs::path path{};
        int host_fd{-1};
        inode_ptr node{};
        bool read{};
        bool write{};

        // add native file handle

        static fd_handle_ptr Create()
        {
            return std::shared_ptr<File>(new File());
        }
    };

}
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

    // resolve path into (parent_dir, leaf_name, inode)
    struct Resolved
    {
        partition_ptr mountpoint{};
        dir_ptr parent{};
        inode_ptr node{}; // nullptr if doesn't exist
        std::string leaf{};
    };

    // sys/stat.h
    struct quasi_stat_t
    {
        dev_t st_dev;         /* Device.  */
        ino_t st_ino;         /* File serial number.	*/
        nlink_t st_nlink;     /* Link count.  */
        mode_t st_mode;       /* File mode.  */
                              //   __uid_t st_uid;		/* User ID of the file's owner.	*/
                              //   __gid_t st_gid;		/* Group ID of the file's group.*/
                              //   __dev_t st_rdev;		/* Device number, if device.  */
        off_t st_size;        /* Size of file, in bytes.  */
        blksize_t st_blksize; /* Optimal block size for I/O.  */
        blkcnt_t st_blocks;   /* Number 512-byte blocks allocated. */

        struct timespec st_atim; /* Time of last access.  */
        struct timespec st_mtim; /* Time of last modification.  */
        struct timespec st_ctim; /* Time of last status change.  */
#define st_atime st_atim.tv_sec  /* Backward compatibility.  */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
    };

    using Stat = struct quasi_stat_t;

    typedef struct File File;
    using fd_handle_ptr = std::shared_ptr<File>;

    struct File
    {
        File() = default;
        ~File() = default;
        fs::path path{};  // qfs path
        int host_fd{-1};  // fd if used with host_io
        inode_ptr node{}; // inode
        bool read{};      // read permission
        bool write{};     // write permission
        off_t pos{};      // cursor offset

        static fd_handle_ptr Create()
        {
            return std::shared_ptr<File>(new File());
        }
    };

    enum class SeekOrigin : uint8_t
    {
        ORIGIN,
        CURRENT,
        END
    };

}
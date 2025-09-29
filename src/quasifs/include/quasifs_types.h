#pragma once

#include <chrono>
#include <filesystem>
#include <vector>

#include <sys/types.h>

namespace QuasiFS
{

    //
    // Filesystem fundamentals
    //

    namespace fs = std::filesystem;

    // internal
    using fileno_t = int64_t;
    using time_point = std::chrono::system_clock::time_point;
    using blkid_t = uint64_t;

    // POSIX-port
    using quasi_errno_t = int;
    using quasi_mode_t = int;
    using quasi_off_t = signed int;
    using quasi_off64_t = int64_t;
    using quasi_lquasi_off_t = int64_t;
    using quasi_ssize_t = long;
    using quasi_size_t = unsigned long;

    using quasi_dev_t = int;
    using quasi_ino_t = int;
    using quasi_nlink_t = int;

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
        partition_ptr mountpoint{}; // target partition
        fs::path local_path{};      // partition's local path
        dir_ptr parent{};           // parent directory
        inode_ptr node{};           // leaf - very last element of the path (if exists, otherwise nullptr)
        std::string leaf{};         // leaf - name
    };

    // sys/stat.h
    struct quasi_stat_t
    {
        quasi_dev_t st_dev;     /* Device.  */
        quasi_ino_t st_ino;     /* File serial number.	*/
        quasi_nlink_t st_nlink; /* Link count.  */
        quasi_mode_t st_mode;   /* File mode.  */
                                //   __uid_t st_uid;		/* User ID of the file's owner.	*/
                                //   __gid_t st_gid;		/* Group ID of the file's group.*/
                                //   __dev_t st_rdev;		/* Device number, if device.  */
        quasi_off_t st_size;    /* Size of file, in bytes.  */
        blksize_t st_blksize;   /* Optimal block size for I/O.  */
        blkcnt_t st_blocks;     /* Number 512-byte blocks allocated. */

        struct timespec st_atim; /* Time of last access.  */
        struct timespec st_mtim; /* Time of last modification.  */
        struct timespec st_ctim; /* Time of last status change.  */
#define st_atime st_atim.tv_sec  /* Backward compatibility.  */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
    };

    // using Stat = struct quasi_stat_t;

    typedef struct File File;
    using fd_handle_ptr = std::shared_ptr<File>;

    struct File
    {
        File() = default;
        ~File() = default;
        int host_fd{-1};         // fd if opened with HostIO
        inode_ptr node{nullptr}; // inode
        bool read{false};        // read permission
        bool write{false};       // write permission
        quasi_off_t pos{0};      // cursor offset

        static fd_handle_ptr Create()
        {
            return std::shared_ptr<File>(new File());
        }

        bool IsOpen(void)
        {
            return nullptr != this->node;
        }
    };

#pragma pack(push, 1)
    typedef struct dirent_t
    {
        quasi_ino_t d_ino{};
        quasi_off_t d_off{};
        unsigned short d_reclen{};
        unsigned char d_type{};
        char d_name[256]{};
    } dirent_t;
#pragma pack(pop)

    enum class SeekOrigin : uint8_t
    {
        ORIGIN,
        CURRENT,
        END
    };

    //
    // Access
    //

    namespace User
    {
        enum
        {
            USER_OWNER = 0x07 << 6,
            USER_GROUP = 0x07 << 3,
            USER_OTHER = 0x07 << 0
        };
    }

    namespace MountOptions
    {
        enum
        {
            MOUNT_NOOPT = 0,
            MOUNT_BIND = 0x01,
            MOUNT_RW = 0x02,     // 0 - ro
            MOUNT_EXEC = 0x04,   // 0 - noexec
            MOUNT_REMOUNT = 0x08 // update mount flags
        };
    }

    typedef struct mount_t
    {
        // path the partition
        fs::path mounted_at;
        dir_ptr parent;
        unsigned int options;
    } mount_t;
}
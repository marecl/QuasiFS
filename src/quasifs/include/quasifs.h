#pragma once

#include <unordered_map>

#include "../../hostio/include/host_io.h"
#include "quasifs_types.h"

/**
 * Wrapper class
 * Basically a Partition object with a bit of extra functionality and single superblock
 */

namespace QuasiFS
{

    // Very small QFS manager: path resolution, mount, create/unlink
    class QFS : public HostIOBase
    {
        // root partition
        partition_ptr rootfs;
        // root directory of the root partition ("/")
        dir_ptr root;

        // inode which holds partitions root in its mounted_root, mountpoint info
        // this allows us to correlate parent/root inodes with corresponding mount options
        // this will make a lot of sense when using RO filesystem opt
        std::unordered_map<partition_ptr, mount_t> block_devices{};

        // open file descriptors. search is linear, looking for first available nullptr
        std::vector<fd_handle_ptr> open_fd;

        HostIO hio_driver{};
        HostVIO vio_driver{};

    public:
        QFS(const fs::path &host_path = "");
        ~QFS() = default;

        //
        // QFS methods
        //

        // Sync mounted partitions with host directories
        int SyncHost(void);
        // Return root directory
        dir_ptr GetRoot() { return this->root; }
        // Return root partition
        partition_ptr GetRootFS() { return this->rootfs; }
   
        // mount fs at path (target must exist and be directory)
        int Mount(const fs::path &path, partition_ptr fs, unsigned int options = MountOptions::MOUNT_NOOPT);
        // mount fs at path (target must exist and be directory)
        int Unmount(const fs::path &path);
        
        /**
         * Resolve path
         * This is kind of a headache, so TLDR:
         *  1. Parent is:
         *      * nullptr if resolution failed within the path, i.e. not on the last element
         *          So /dir/dir_not_exist/dir2 wil return both nulls, while /dir/dir2/file_not_exist
         *          will return all good with nullptr node
         *      * Last available inode
         *      * Inode that has something mounted (always a dir)
         *  2. Node is:
         *      * nullptr if target is not found
         *      * target inode (if found, irrespectable of type)
         *      * Inode that's mounted (always a dir)
         *  3. Mountpoint is:
         *      * Partition owning the inode
         *      * Never null (if ENOENT, points at last valid partition)
         *  4. Local path is:
         *      * Full path in the beginning
         *      * Trimmed from the front when entering partition
         *          (absolute in regards to root of the partition)
         *      * Updated when symlink is encountered
         *      * Used only for: path resolution, extracting host-bound path
         *  5. Leaf is:
         *      * Never empty
         *      * Name of the last element
         *          * path.filename() if found OR
         *          * name of the element that's NOT found
         *  6. On mountpoint resolution:
         *      * leaf - last element (no change)
         *      * mountpoint - mountpoint (no change)
         *      * parent - holding dir that is a mountpoint (/dir/INODE->mounted_dir)
         *      * node - node holding mounted root (/dir/inode->MOUNTED_DIR)
         */
        int Resolve(const fs::path &path, Resolved &r);


        //
        // Inherited from HostIOBase
        // Raw file operations
        // man(2)
        //
        
        int Open(const fs::path &path, int flags, quasi_mode_t mode = 0755);
        int Creat(const fs::path &path, quasi_mode_t mode = 0755);
        int Close(const int fd);
        int LinkSymbolic(const fs::path &src, const fs::path &dst);
        int Link(const fs::path &src, const fs::path &dst);
        int Unlink(const fs::path &path);
        // int Flush(const int fd);
        // int FSync(const int fd);
        int Truncate(const fs::path &path, quasi_size_t size);
        int FTruncate(const int fd, quasi_size_t size);
        quasi_off_t LSeek(const int fd, quasi_off_t offset, SeekOrigin origin);
        quasi_ssize_t Tell(const int fd);
        quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count);
        quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset);
        quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count);
        quasi_ssize_t PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset);
        int MKDir(const fs::path &path, quasi_mode_t mode = 0755);
        int RMDir(const fs::path &path);

        int Stat(const fs::path &path, quasi_stat_t *statbuf);
        int FStat(const int fd, quasi_stat_t *statbuf);

        //
        // Additional binds
        //

        quasi_ssize_t getdirectorysize(const fs::path &path) { return -QUASI_EINVAL; };
        int IsOpen(const int fd) noexcept { return -QUASI_EINVAL; };
        int SetSize(const int fd, uint64_t size) noexcept { return -QUASI_EINVAL; };
        quasi_ssize_t GetSize(const int fd) noexcept { return -QUASI_EINVAL; };
        // Not a port, used by 2-3 functions that *never* check for errors
        quasi_ssize_t GetDirectorySize(const fs::path &path) noexcept { return -QUASI_EINVAL; };

        //
        // C++ ports with both except/noexcept
        //

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
        void SyncHostImpl(partition_ptr part, const fs::path &dir, std::string prefix = "");

        // Get next available fd slot
        int GetFreeHandleNo();
        fd_handle_ptr GetHandle(int fd);
        // partition by blkdev
        //  partition_ptr GetPartitionByBlockdev(uint64_t blkid);
        mount_t *GetPartitionInfo(partition_ptr part);
        partition_ptr GetPartitionByPath(fs::path path);
        partition_ptr GetPartitionByParent(dir_ptr dir);
        int IsPartitionRO(partition_ptr part);
    };

};
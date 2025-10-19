// INAA License @marecl 2025

#pragma once

#include <unordered_map>

#include "quasi_sys_stat.h"
#include "quasi_types.h"
#include "quasifs_inode.h"
#include "quasifs_inode_directory.h"
#include "quasifs_inode_symlink.h"

#include "../hostio/host_io.h"

/**
 * Wrapper class
 * Basically a Partition object with a bit of extra functionality and single superblock
 */

namespace QuasiFS
{
    std::string file_mode(quasi_mode_t mode);

    void _printTree(const inode_ptr &node, const std::string &name, int depth);

    void printTree(const dir_ptr &node, const std::string &name, int depth = 0);

    using namespace Stat;

    // Very small QFS manager: path resolution, mount, create/unlink
    class QFS
    {
    private:
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

        //
        // Inherited from HostIOBase
        // Native filesystem operations, here because top-level FS *must* interact with every step
        // when in doubt - man(2)
        //

        class OperationImpl : public HostIO::HostIO_Base
        {
        private:
            QFS &qfs;
            OperationImpl &operator=(const OperationImpl &) = delete;

        public:
            OperationImpl(QFS &qfs) : qfs(qfs) {}
            int Open(const fs::path &path, int flags, quasi_mode_t mode = 0755) override;
            int Creat(const fs::path &path, quasi_mode_t mode = 0755) override;
            int Close(const int fd) override;
            int LinkSymbolic(const fs::path &src, const fs::path &dst) override;
            int Link(const fs::path &src, const fs::path &dst) override;
            int Unlink(const fs::path &path) override;
            int Flush(const int fd) override;
            int FSync(const int fd) override;
            int Truncate(const fs::path &path, quasi_size_t size) override;
            int FTruncate(const int fd, quasi_size_t size) override;
            quasi_off_t LSeek(const int fd, quasi_off_t offset, SeekOrigin origin) override;
            quasi_ssize_t Tell(const int fd) override;
            quasi_ssize_t Write(const int fd, const void *buf, quasi_size_t count) override;
            quasi_ssize_t PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset) override;
            quasi_ssize_t Read(const int fd, void *buf, quasi_size_t count) override;
            quasi_ssize_t PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset) override;
            int MKDir(const fs::path &path, quasi_mode_t mode = 0755) override;
            int RMDir(const fs::path &path) override;

            int Stat(const fs::path &path, quasi_stat_t *statbuf) override;
            int FStat(const int fd, quasi_stat_t *statbuf) override;

            int Chmod(const fs::path &path, quasi_mode_t mode) override;
            int FChmod(const int fd, quasi_mode_t mode) override;
        };

    public:
        QFS(const fs::path &host_path = "");
        ~QFS() = default;

        //
        // QFS methods
        //

        // Sync mounted partitions with host directories
        int SyncHost(void);
        int SyncHost(fs::path path);
        // Return root directory
        dir_ptr GetRoot() { return this->root; }
        // Return root partition
        partition_ptr GetRootFS() { return this->rootfs; }

        // mount fs at path (target must exist and be directory)
        // empty options imply read-only
        int Mount(const fs::path &path, partition_ptr fs, unsigned int options = MountOptions::MOUNT_NOOPT);
        // unmount fs from path
        int Unmount(const fs::path &path);

        // may or may not break stuff (duuun dun)
        int ForceInsert(const fs::path &path, const std::string &name, inode_ptr node);

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
         *      * Null when partition is not found
         *  4. Local path is:
         *      * Always set by partition's resolve
         *      * Every leaf that was encountered on resolution step
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
        int Resolve(const fs::path &path, Resolved &res);

        int GetHostPath(fs::path &output, const fs::path &path = "/");

        //
        // Additional binds
        //

        bool IsOpen(const int fd) noexcept;
        int SetSize(const int fd, uint64_t size) noexcept;
        quasi_ssize_t GetSize(const int fd) noexcept;
        // Not a port, used by 2-3 functions that ;
        quasi_ssize_t GetDirectorySize(const fs::path &path) noexcept;

        //
        // FS operations
        // I think this class is huge already, so OG fs ops are moved to a separate "namespace"
        //

        OperationImpl Operation{*this};

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
        void SyncHostImpl(partition_ptr part);

        // Get next available fd slot
        int GetFreeHandleNo();
        fd_handle_ptr GetHandle(int fd);
        // partition by blkdev
        //  partition_ptr GetPartitionByBlockdev(uint64_t blkid);
        mount_t *GetPartitionInfo(const partition_ptr part);
        partition_ptr GetPartitionByPath(const fs::path &path);
        partition_ptr GetPartitionByParent(const dir_ptr dir);
        int IsPartitionRO(const partition_ptr part);
    };

};
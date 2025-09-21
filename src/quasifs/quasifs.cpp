
#include "quasi_errno.h"
#include "include/quasifs_types.h"

#include "include/quasifs_inode_directory.h"
#include "include/quasifs_inode_regularfile.h"
#include "include/quasifs_inode_symlink.h"
#include "include/quasifs_partition.h"
#include "include/quasifs.h"

#include "../log.h"
namespace QuasiFS
{

    QFS::QFS()
    {
        this->rootfs = Partition::Create();
        this->root = rootfs->GetRoot();
        this->block_devices[this->rootfs->GetBlkId()] = this->rootfs;
    }

    int QFS::Resolve(fs::path path, Resolved &r)
    {
        // on return:
        // node - last element of the path (if exists)
        // parent - parent element of the path (if parent dir is 1 level above last element)
        // mountpoint - target partition
        // leaf - name of the last element in the path (if exists)

        // guard against circular binds
        uint8_t safety_counter = 40;
        //
        int status{-1};

        r.mountpoint = this->rootfs;

        do
        {
            status = r.mountpoint->Resolve(path, r);

            if (0 != status)
                return status;

            if (r.node->is_link())
            {
                // symlink holds absolute path, so we need to start over.
                // it may be a file or a directory, but if it's in the middle - it has to be a directory.
                // when it's a part of the path, `Resolve` returns null parent (it's unused anyways), and leaf is
                // remaining (uniterated) path.

                // for example, there are 2 paths:
                // /dirA/dirB/dirC/file.txt
                // /dirD/sym [sym to dirB]
                // we open /dirD/sym/dirC/tile.txt, so base path is substituted with /dirA/dirB + /dirC/file.txt
                // incorrect symlink will just throw -QUASI_ENOTDIR

                path = std::static_pointer_cast<Symlink>(r.node)->follow() / path;

                r.mountpoint = this->rootfs;
                r.parent = this->root;
                r.node = this->root;
                r.leaf = "/";
                continue;
            }

            if (r.node->is_dir())
            {
                dir_ptr mntparent = r.parent;
                dir_ptr mntroot = mntparent->mounted_root;

                if (nullptr != mntroot)
                {
                    // nothing mounted

                    // cross-partition resolution returns last-local directory, that holds the
                    // mounted fs root directory. target fs is determined by mounted dir st.blk (ie. physical device).
                    // here .leaf holds remainder of the path, that can be used as a starting point to iterate in new partition.

                    auto mounted_blkdev = mntroot->getattr().st_dev;

                    partition_ptr mounted_partition = GetPartitionByBlockdev(mounted_blkdev);
                    if (nullptr == mounted_partition)
                        return -QUASI_ENOENT;

                    r.mountpoint = mounted_partition;
                    r.parent = mntparent;
                    r.node = mntroot;

                    if (!path.empty())
                        continue;
                }
            }

            break;

        } while (--safety_counter > 0);

        if (0 == safety_counter)
            return -QUASI_ELOOP;

        return 0;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::Mount(const fs::path &path, partition_ptr fs)
    {
        if (nullptr != GetPartitionByBlockdev(fs->GetBlkId()))
            return -QUASI_EEXIST;

        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        if (!res.node->is_dir())
            return -QUASI_ENOTDIR;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        if (dir->mounted_root)
            return -QUASI_EEXIST;

        dir->mounted_root = fs->GetRoot();
        this->block_devices[fs->GetBlkId()] = fs;

        return 0;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::Unmount(const fs::path &path)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        if (nullptr == GetPartitionByBlockdev(res.mountpoint->GetBlkId()))
            return -QUASI_EINVAL;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.parent);

        if (nullptr == dir->mounted_root)
            // mounted but rootdir disappeared O.o
            return -QUASI_EINVAL;

        dir->mounted_root = nullptr;
        this->block_devices.erase(res.mountpoint->GetBlkId());

        return 0;
    }

    int QFS::MapHost(const fs::path &local, const fs::path &host)
    {
        Resolved r{};
        int status = Resolve(local, r);

        if (0 != status)
            return status;


        const fs::path existing_host_path = GetHostPathByInode(r.node);

        if (!existing_host_path.empty())
            return -EEXIST;

        Log("Mapping local {} to hosts {}", local.string(), host.string());
        this->host_files[r.node] = host;
        return 0;
    }

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    int QFS::Touch(const fs::path &path)
    {
        fs::path base = path.parent_path();
        std::string fname = path.filename();

        return Touch(base, fname);
    }

    int QFS::Touch(const fs::path &path, const std::string &name)
    {
        return Touch(path, name, RegularFile::Create());
    }

    int QFS::Touch(const fs::path &path, const std::string &name, file_ptr child)
    {
        Resolved res{};
        int ret = Resolve(path, res);

        if (0 != ret)
            return ret;

        partition_ptr fsblk = res.mountpoint;
        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        return fsblk->touch(dir, name, child);
    }

    int QFS::MKDir(const fs::path &path, const std::string &name)
    {
        return MKDir(path, name, Directory::Create());
    }

    int QFS::MKDir(const fs::path &path, const std::string &name, dir_ptr child)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        if (!res.node->is_dir())
            return -QUASI_ENOTDIR;

        partition_ptr fsblk = res.mountpoint;
        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        return fsblk->mkdir(dir, name, child);
    }

    int QFS::GetFreeHandleNo()
    {
        auto open_fd_size = open_fd.size();
        for (size_t idx = 0; idx < open_fd_size; idx++)
        {
            if (this->open_fd[idx] == nullptr)
                return idx;
        }
        open_fd.push_back(nullptr);
        return open_fd_size;
    }

    partition_ptr QFS::GetPartitionByBlockdev(uint64_t blkid)
    {
        auto target_blkdev = this->block_devices.find(blkid);
        // already mounted
        if (target_blkdev == this->block_devices.end())
            return nullptr;
        return target_blkdev->second;
    }

    fs::path QFS::GetHostPathByPath(const fs::path &path)
    {
        Resolved r{};
        if (0 != this->Resolve(path, r))
            return {};
        return GetHostPathByInode(r.node);
    }

    fs::path QFS::GetHostPathByInode(inode_ptr node)
    {
        auto target_path = this->host_files.find(node);
        if (target_path == this->host_files.end())
            return {};
        return target_path->second;
    }

};
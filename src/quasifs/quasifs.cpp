
#include "quasi_errno.h"
#include "include/quasifs_types.h"

#include "include/quasifs.h"

#include "include/quasifs_inode_directory.h"
#include "include/quasifs_inode_regularfile.h"
#include "include/quasifs_inode_symlink.h"
#include "include/quasifs_partition.h"

#include "../log.h"

namespace QuasiFS
{

    QFS::QFS(const fs::path &host_path)
    {
        this->rootfs = Partition::Create(host_path);
        this->root = rootfs->GetRoot();
        this->block_devices[this->rootfs->GetBlkId()] = this->rootfs;
    }

    void QFS::SyncHostImpl(partition_ptr &part, const fs::path &dir, std::string prefix)
    {
        fs::path host_path{};
        if (0 != part->GetHostPath(host_path))
        {
            LogError("Cannot safely resolve host directory for blkdev {}", part->GetBlkId());
            return; // false
        }

        // cut out host-root, remainder is Partition path
        auto host_path_components = std::distance(host_path.begin(), host_path.end()) - 1;
        auto slice_path = [host_path_components](const fs::path &p)
        {
            fs::path out;
            auto it = p.begin();
            std::advance(it, host_path_components);
            for (; it != p.end(); ++it)
                out /= *it;
            return out;
        };

        try
        {
            for (auto entry = fs::recursive_directory_iterator(host_path); entry != fs::recursive_directory_iterator(); entry++)
            {
                // wcięcie zależne od głębokości
                fs::path entry_path = entry->path();
                fs::path pp = "/" / slice_path(entry->path());
                fs::path parent_path = pp.parent_path();
                fs::path leaf = pp.filename();

                Resolved r{};
                part->Resolve(parent_path, r);

                if (nullptr == r.node)
                {
                    LogError("Cannot resolve quasi-target for sync: {}", parent_path.string());
                    continue;
                }

                dir_ptr parent_dir = r.node->is_dir() ? std::static_pointer_cast<Directory>(r.node) : nullptr;
                inode_ptr new_inode{};

                if (entry->is_directory())
                {
                    new_inode = Directory::Create();
                    part->mkdir(parent_dir, leaf, std::static_pointer_cast<Directory>(new_inode));
                }
                else if (entry->is_regular_file())
                {
                    new_inode = RegularFile::Create();
                    part->touch(parent_dir, leaf, std::static_pointer_cast<RegularFile>(new_inode));
                }
                else
                {
                    LogError("Unsupported host file type: {}", entry_path.string());
                    continue;
                }

                if (0 != this->hio_driver.Stat(entry_path, &new_inode->st))
                {
                    LogError("Cannot stat file: {}", entry_path.string());
                    continue;
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Błąd: " << e.what() << "\n";
        }
        return; // true
    }

    int QFS::SyncHost(void)
    {
        for (auto &[blkid, part] : this->block_devices)
        {
            if (part->IsHostMounted())
                SyncHostImpl(part, "/");
        }

        return 0;
    }

    int QFS::Resolve(fs::path path, Resolved &r)
    {
        if (path.empty())
            return -QUASI_EINVAL;
        if (path.is_relative())
            return -QUASI_EBADF;

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
        r.local_path = path;
        r.parent = this->root;
        r.node = this->root;

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
                    r.local_path = path;
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

    int QFS::GetFreeHandleNo()
    {
        auto open_fd_size = open_fd.size();
        for (size_t idx = 0; idx < open_fd_size; idx++)
        {
            if (nullptr == this->open_fd[idx])
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

};
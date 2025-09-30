
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

        mount_t mount_options = {
            .mounted_at{"/"},
            .parent = this->root,
            .options = MountOptions::MOUNT_RW,
        };

        this->block_devices[this->rootfs] = mount_options;
    }

    int QFS::SyncHost(void)
    {
        for (auto &[part, info] : this->block_devices)
        {
            if (part->IsHostMounted())
                SyncHostImpl(part, "/");
        }

        return 0;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::Mount(const fs::path &path, partition_ptr fs, unsigned int options)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        if (!res.node->is_dir())
            return -QUASI_ENOTDIR;

        if (nullptr != GetPartitionInfo(fs))
        {
            LogError("Partition cannot be mounted twice");
            return -QUASI_EEXIST;
        }

        if (options & MountOptions::MOUNT_BIND)
            LogError("Mount --bind not implemented");

        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        if (dir->mounted_root)
            return -QUASI_EEXIST;

        dir_ptr fs_root = fs->GetRoot();
        mount_t fs_options = {
            .mounted_at = path,
            .parent = dir,
            .options = options,
        };

        dir->mounted_root = fs_root;
        this->block_devices[fs] = fs_options;

        return 0;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::Unmount(const fs::path &path)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        partition_ptr part = res.mountpoint;
        mount_t *part_opts = GetPartitionInfo(part);

        if (nullptr == part_opts)
            return -QUASI_EINVAL;

        dir_ptr dir = std::static_pointer_cast<Directory>(part_opts->parent);

        if (dir != res.parent)
            LogError("XDXDXD");

        if (nullptr == dir->mounted_root)
            // mounted but rootdir disappeared O.o
            return -QUASI_EINVAL;

        dir->mounted_root = nullptr;
        this->block_devices.erase(part);

        return 0;
    }

    int QFS::Resolve(const fs::path &path, Resolved &r)
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

        fs::path iter_path = path;

        r.mountpoint = this->rootfs;
        r.local_path = iter_path;
        r.parent = this->root;
        r.node = this->root;

        do
        {
            status = r.mountpoint->Resolve(iter_path, r);

            if (0 != status)
                return status;

            if (r.node->is_link())
            {
                // symlinks consume path from the front, since they point to an absolute location
                // let's say /link is linked to /dirA/dirB, and we need to resolve /link/dirC
                // path resolution will enter /link, and extract it as /dirA/dirB.
                // from that same path, /dirC will be preserved and appened to symlink's target,
                // which will yield /dirA/dirB/dirC
                fs::path leftover = iter_path;
                // main path is overwritten with absolute path from symlink
                iter_path = std::static_pointer_cast<Symlink>(r.node)->follow();
                // and if it's really in the way - restore leftover items
                if (!leftover.empty())
                    iter_path /= leftover;
                // reset everything to point to rootfs, where absolute path can be resolved again
                r.mountpoint = this->rootfs;
                r.parent = this->root;
                r.node = this->root;
                r.leaf = "/";
                continue;
            }

            if (r.node->is_dir())
            {
                dir_ptr mntparent = r.parent;
                inode_ptr mntroot = mntparent->mounted_root;

                if (nullptr != mntroot)
                {
                    if (mntroot != r.node)
                        LogError("Resolved conflicting mount root and node");

                    // just like symlinks, only trailing path is saved
                    // directory, in which partition is mounted, belongs to upstream filesystem,
                    // so everything before (including) that directory is consumed

                    partition_ptr mounted_partition = GetPartitionByParent(mntparent);

                    if (nullptr == mounted_partition)
                        return -QUASI_ENOENT;

                    r.mountpoint = mounted_partition;
                    r.local_path = iter_path;
                    // set by partition resolve
                    r.parent = mntparent;
                    r.node = mntroot;

                    if (!iter_path.empty())
                        continue;
                }
            }

            break;

        } while (--safety_counter > 0);

        if (0 == safety_counter)
            return -QUASI_ELOOP;

        return 0;
    }

    void QFS::SyncHostImpl(partition_ptr part, const fs::path &dir, std::string prefix)
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

    fd_handle_ptr QFS::GetHandle(int fd)
    {
        if (fd < 0 || fd >= this->open_fd.size())
            return nullptr;
        return this->open_fd.at(fd);
    }

    mount_t *QFS::GetPartitionInfo(partition_ptr part)
    {
        auto target_part_info = this->block_devices.find(part);
        // already mounted
        if (target_part_info == this->block_devices.end())
            return nullptr;
        return &(target_part_info->second);
    }

    partition_ptr QFS::GetPartitionByPath(fs::path path)
    {
        for (auto &[part, info] : this->block_devices)
        {
            if (info.mounted_at == path)
                return part;
        }
        return nullptr;
    }

    partition_ptr QFS::GetPartitionByParent(dir_ptr dir)
    {
        for (auto &[part, info] : this->block_devices)
        {
            if (info.parent == dir)
                return part;
        }
        return nullptr;
    }

    int QFS::IsPartitionRO(partition_ptr part)
    {
        mount_t *part_info = GetPartitionInfo(part);
        if (nullptr == part_info)
            return -QUASI_ENODEV;
        if (part_info->options & MountOptions::MOUNT_RW)
            return 0;
        return 1;
    }
};
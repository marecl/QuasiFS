
#include "include/quasi_types.h"
#include "include/quasi_inode_directory.h"
#include "include/quasi_inode_regularfile.h"
#include "include/quasi_inode_symlink.h"
#include "include/quasi_partition.h"
#include "include/quasi_fs.h"

namespace QuasiFS
{

    QFS::QFS()
    {
        this->rootfs = Partition::Create();
        this->root = rootfs->GetRoot();
        this->block_devices[this->rootfs->GetBlkId()] = this->rootfs;
    }

    Resolved QFS::resolve(const fs::path &path)
    {
        auto is_symlink = this->symlink_table.find(path);
        if (is_symlink != this->symlink_table.end())
        {
            symlink_ptr sym = is_symlink->second;
            return resolve(sym->follow());
        }

        return this->rootfs->resolve(path);

        // Resolved res{};

        // if (path.empty() || path.is_relative())
        //     return res; // tylko ścieżki absolutne

        // res.mountpoint = this->rootfs;
        // dir_ptr parent_inode = this->root;
        // inode_ptr current_inode = this->root;
        // std::string leaf{};

        // for (auto it = path.begin(); it != path.end(); ++it)
        // {
        //     leaf = *it;
        //     if (leaf.empty() || leaf == "/")
        //         continue;

        //     res.leaf = leaf;

        //     dir_ptr dir = std::dynamic_pointer_cast<Directory>(current_inode);
        //     if (!dir)
        //     {
        //         // próbujemy iść w dół w pliku
        //         res.parent = parent_inode;
        //         res.node = current_inode;
        //         return res;
        //     }

        //     // jeśli katalog jest mountpointem, delegujemy do mounted_root
        //     if (dir->mounted_root)
        //     {
        //         auto mntpoint = this->fs_table.find(dir->GetFileno());
        //         if (mntpoint == this->fs_table.end())
        //             return {};

        //         res.mountpoint = mntpoint->second;
        //         parent_inode = mntpoint->second->GetRoot();
        //     }
        //     else
        //     {
        //         parent_inode = std::static_pointer_cast<Directory>(current_inode);
        //     }

        //     current_inode = dir->lookup(leaf);
        //     if (std::next(it) == path.end())
        //     {
        //         // ostatni element ścieżki
        //         res.parent = parent_inode;
        //         res.leaf = leaf;
        //         res.node = current_inode;

        //         return res;
        //     }

        //     if (!current_inode)
        //     {
        //         // ścieżka nie istnieje
        //         res.parent = nullptr;
        //         res.node = nullptr;

        //         return res;
        //     }
        // }

        // // jeśli ścieżka to "/", zwracamy root
        // res.parent = root;
        // res.node = root;
        // res.leaf = "/";

        // return res;
    }

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    int QFS::touch(const fs::path &path)
    {
        Resolved res = resolve(path);
        if (!res.parent)
            return -ENOENT;
        if (res.node)
            return -EEXIST;
        if (!res.parent->is_dir())
            return -ENOTDIR;
        if (!res.mountpoint)
            return -ENOENT;

        partition_ptr fsblk = res.mountpoint;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.parent);

        return fsblk->touch(dir, res.leaf);
    }

    // Note: target may not exist and symlink will be created
    int QFS::symlink(const fs::path symlink_location, const fs::path target)
    {
        Resolved r = resolve(symlink_location);

        if (!r.mountpoint)
            return -ENOENT;
        if (!r.parent)
            return -ENOENT;
        if (r.node)
            // something exists in symlinks place
            return -EEXIST;

        auto exists = this->symlink_table.find(symlink_location);
        if (exists == this->symlink_table.end())
            // target file doesn;t exist
            return -ENOENT;

        symlink_ptr new_symlink = Inode::Create<Symlink>(target);

        this->symlink_table[symlink_location] = new_symlink;

        return 0;
    }

    int QFS::mkdir(const fs::path &path)
    {
        Resolved res = resolve(path);
        if (!res.parent)
            return -ENOENT;
        if (res.node)
            return -EEXIST;
        if (!res.parent->is_dir())
            return -ENOTDIR;
        if (!res.mountpoint)
            return -ENOENT;

        partition_ptr fsblk = res.mountpoint;

        return fsblk->mkdir(res.parent, res.leaf);
    }

    int QFS::rmdir(const fs::path &path)
    {
        Resolved res = resolve(path);
        if (!res.parent)
            return -ENOENT;
        if (!res.node)
            return -ENOENT;
        if (!res.parent->is_dir())
            return -ENOTDIR;
        if (!res.mountpoint)
            return -ENOENT;

        auto dir = std::static_pointer_cast<Directory>(res.parent);

        if (res.parent->mounted_root)
            return -EBUSY;

        return dir->unlink(res.leaf);
    }

    int QFS::link(const fs::path what, const fs::path where)
    {
        Resolved sos = resolve(what);
        Resolved tar = resolve(where);

        // cross-partition linking is not supported yet
        if (sos.mountpoint != tar.mountpoint)
            return -EINVAL;

        if (!sos.node)
            return -ENOENT;
        if (tar.node)
            return -EEXIST;

        inode_ptr sos_inode = sos.node;
        return tar.parent->link(sos_inode, tar.leaf);
    }

    int QFS::unlink(const std::string &path)
    {
        Resolved res = resolve(path);
        if (!res.parent)
            return -ENOENT;
        if (!res.node)
            return -ENOENT;
        if (!res.parent->is_dir())
            return -ENOTDIR;

        auto dir = std::static_pointer_cast<Directory>(res.parent);

        if (0 != dir->unlink(res.leaf))
            return -EINVAL;

        if (res.node->st.nlink != 0)
            return 0;

        // dunno
        return -EINVAL;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::mount(const fs::path &target_path, partition_ptr fs)
    {
        auto target_blkdev = this->block_devices.find(fs->GetBlkId());
        // already mounted
        if (target_blkdev != this->block_devices.end())
            return -EEXIST;

        Resolved res = resolve(target_path);

        if (!res.node)
            return -ENOENT;
        if (!res.node->is_dir())
            return -ENOTDIR;
        if (!res.mountpoint)
            return -EINVAL;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        if (dir->mounted_root)
            return -EEXIST;

        dir->mounted_root = fs->GetRoot();
        res.mountpoint->fs_table[dir->GetFileno()] = fs;
        this->block_devices[fs->GetBlkId()] = fs;

        return 0;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::unmount(const fs::path &target_path)
    {
        Resolved res = resolve(target_path);
        if (!res.node)
            return -ENOENT;
        if (!res.node->is_dir())
            return -ENOTDIR;
        if (!res.mountpoint)
            return -EINVAL;

        auto target_blkdev = this->block_devices.find(res.mountpoint->GetBlkId());
        if (target_blkdev == this->block_devices.end())
            // not mounted
            return -EINVAL;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        if (nullptr == dir->mounted_root)
            // mounted but rootdir disappeared O.o
            return -EINVAL;

        dir->mounted_root = nullptr;
        res.mountpoint->fs_table.erase(dir->GetFileno());
        this->block_devices.erase(res.mountpoint->GetBlkId());

        return 0;
    }

};
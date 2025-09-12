
#include "include/quasi_types.h"
#include "include/quasi_inode_directory.h"
#include "include/quasi_inode_regularfile.h"
#include "include/quasi_partition.h"
#include "include/quasi_fs.h"

namespace QuasiFS
{

    QFS::QFS()
    {
        this->rootfs = std::make_shared<Partition>();
        this->rootfs->GetBlkId();
        this->root = rootfs->GetRoot();

        this->fs_table[this->root->GetFileno()] = rootfs;
    }

    Resolved QFS::resolve(const fs::path &path)
    {
        Resolved res{};

        if (path.empty() || path.is_relative())
            return res; // tylko ścieżki absolutne

        res.mountpoint = this->rootfs;
        dir_ptr parent_inode = this->root;
        inode_ptr current_inode = this->root;
        std::string leaf{};

        for (auto it = path.begin(); it != path.end(); ++it)
        {
            leaf = *it;
            if (leaf.empty() || leaf == "/")
                continue;

            res.leaf = leaf;

            dir_ptr dir = std::dynamic_pointer_cast<Directory>(current_inode);
            if (!dir)
            {
                // próbujemy iść w dół w pliku
                res.parent = parent_inode;
                res.node = current_inode;
                return res;
            }

            // jeśli katalog jest mountpointem, delegujemy do mounted_root
            if (dir->mounted_root)
            {
                auto mntpoint = this->fs_table.find(dir->GetFileno());
                if (mntpoint == this->fs_table.end())
                    return {};

                res.mountpoint = mntpoint->second;
                parent_inode = mntpoint->second->GetRoot();
            }
            else
            {
                parent_inode = std::static_pointer_cast<Directory>(current_inode);
            }

            current_inode = dir->lookup(leaf);
            if (std::next(it) == path.end())
            {
                // ostatni element ścieżki
                res.parent = parent_inode;
                res.leaf = leaf;
                res.node = current_inode;

                return res;
            }

            if (!current_inode)
            {
                // ścieżka nie istnieje
                res.parent = nullptr;
                res.node = nullptr;

                return res;
            }
        }

        // jeśli ścieżka to "/", zwracamy root
        res.parent = root;
        res.node = root;
        res.leaf = "/";

        return res;
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
        // zmienic na fsblk link
        // return 0;
        return fsblk->touch(dir, res.leaf);
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
        return dir->unlink(res.leaf);
    }

    // mount fs at path (target must exist and be directory)
    int QFS::mount(const fs::path &target_path, partition_ptr fs)
    {
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
        this->fs_table[dir->GetFileno()] = fs;
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

        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        this->fs_table.erase(dir->GetFileno());
        dir->mounted_root = nullptr;

        return 0;
    }

};
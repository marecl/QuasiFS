
#include "include/quasifs_types.h"

#include "include/quasifs_inode_directory.h"
#include "include/quasifs_inode_regularfile.h"
#include "include/quasifs_inode_symlink.h"
#include "include/quasifs_partition.h"
#include "include/quasifs.h"

namespace QuasiFS
{

    QFS::QFS()
    {
        this->rootfs = Partition::Create();
        this->root = rootfs->GetRoot();
        this->block_devices[this->rootfs->GetBlkId()] = this->rootfs;
    }

    int QFS::resolve(fs::path path, Resolved &r)
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
            status = r.mountpoint->resolve(path, r);

            if (0 != status)
                return status;

            if (r.node->is_link())
            {
                // symlink holds absolute path, so we need to start over.
                // it may be a file or a directory, but if it's in the middle - it has to be a directory.
                // when it's a part of the path, `resolve` returns null parent (it's unused anyways), and leaf is
                // remaining (uniterated) path.

                // for example, there are 2 paths:
                // /dirA/dirB/dirC/file.txt
                // /dirD/sym [sym to dirB]
                // we open /dirD/sym/dirC/tile.txt, so base path is substituted with /dirA/dirB + /dirC/file.txt
                // incorrect symlink will just throw -ENOTDIR

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
                    auto lookup_blkdev = this->block_devices.find(mounted_blkdev);
                    if (lookup_blkdev == this->block_devices.end())
                        // check - device not present. unmounted?
                        return -ENOENT;
                    partition_ptr mounted_partition = lookup_blkdev->second;

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
            return -ELOOP;

        return 0;
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

    int QFS::open(fs::path path, int flags)
    {
        Resolved r{};
        int status = resolve(path, r);

        if (-ENOENT == status)
            if ((flags & O_CREAT) == 0)
                this->touch(path);
        if (-ENOENT != status)
        {
            if (flags & O_EXCL)
                return -EEXIST;
            if (0 != status)
                return status;
        }

        auto next_free_handle = this->GetFreeHandleNo();
        fd_handle_ptr handle = File::Create();

        handle->node = r.node;
        handle->read = flags & O_WRONLY ? false : true;
        handle->write = flags & O_WRONLY ? true : (flags & O_RDWR);

        this->open_fd[next_free_handle] = handle;
        return next_free_handle;
    }

    int QFS::close(int fd)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -EBADF;

        this->open_fd.at(fd) = nullptr;
        return 0;
    }

    ssize_t QFS::write(int fd, const void *buf, size_t count)
    {
        return pwrite(fd, buf, count, 0);
    }

    ssize_t QFS::pwrite(int fd, const void *buf, size_t count, off_t offset)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -EBADF;
        fd_handle_ptr handle = this->open_fd.at(fd);
        if (!handle->write)
            return -EBADF;

        inode_ptr target = handle->node;
        if (target->is_file())
            return std::static_pointer_cast<RegularFile>(target)->write(offset, buf, count);
        // TODO: remaining types
        return -EBADF;
    }

    ssize_t QFS::read(int fd, void *buf, size_t count)
    {
        return pread(fd, buf, count, 0);
    }

    ssize_t QFS::pread(int fd, void *buf, size_t count, off_t offset)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -EBADF;
        fd_handle_ptr handle = this->open_fd.at(fd);
        if (!handle->read)
            return -EBADF;

        inode_ptr target = handle->node;
        if (target->is_file())
            return std::static_pointer_cast<RegularFile>(target)->read(offset, buf, count);
        // TODO: remaining types
        return -EBADF;
    }

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    int QFS::touch(const fs::path &path)
    {
        fs::path base = path.parent_path();
        std::string fname = path.filename();

        return touch(base, fname);
    }

    int QFS::touch(const fs::path &path, const std::string &name)
    {
        return touch(path, name, RegularFile::Create<RegularFile>());
    }

    int QFS::touch(const fs::path &path, const std::string &name, file_ptr child)
    {
        Resolved res{};
        int ret = resolve(path, res);

        if (0 != ret)
            return ret;

        partition_ptr fsblk = res.mountpoint;
        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        return fsblk->touch(dir, name, child);
    }

    // Note: target may not exist and symlink will be created
    int QFS::symlink(const fs::path path, const fs::path target)
    {
        return -EINVAL;
    }

    int QFS::mkdir(const fs::path &path)
    {
        fs::path base = path.parent_path();
        std::string fname = path.filename();

        return mkdir(base, fname);
    }

    int QFS::mkdir(const fs::path &path, const std::string &name)
    {
        return mkdir(path, name, Directory::Create<Directory>());
    }

    int QFS::mkdir(const fs::path &path, const std::string &name, dir_ptr child)
    {
        Resolved res{};
        int status = resolve(path, res);

        if (0 != status)
            return status;

        if (!res.node->is_dir())
            return -ENOTDIR;

        partition_ptr fsblk = res.mountpoint;
        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        return fsblk->mkdir(dir, name, child);
    }

    int QFS::rmdir(const fs::path &path)
    {
        Resolved res{};
        int status = resolve(path, res);

        if (0 != status)
            return status;

        dir_ptr parent = res.parent;

        if (parent->mounted_root)
            return -EBUSY;

        return parent->unlink(res.leaf);
    }

    int QFS::link(const fs::path what, const fs::path where)
    {
        Resolved sos{};
        Resolved tar{};
        int status_what = resolve(what, sos);
        int status_where = resolve(where, tar);

        if (0 != status_what)
            return status_what;
        if (0 == status_where)
            return -EEXIST;

        // cross-partition linking is not supported
        if (sos.mountpoint != tar.mountpoint)
            return -EINVAL;

        inode_ptr sos_inode = sos.node;
        return tar.parent->link(tar.leaf, sos_inode);
    }

    int QFS::unlink(const std::string &path)
    {
        Resolved res{};
        int status = resolve(path, res);

        if (0 != status)
            return status;

        partition_ptr part = res.mountpoint;
        dir_ptr dir = res.parent;
        // inode_ptr target = res.node;

        return part->unlink(dir, res.leaf);
    }

    // mount fs at path (target must exist and be directory)
    int QFS::mount(const fs::path &path, partition_ptr fs)
    {
        auto target_blkdev = this->block_devices.find(fs->GetBlkId());
        // already mounted
        if (target_blkdev != this->block_devices.end())
            return -EEXIST;

        Resolved res{};
        int status = resolve(path, res);

        if (0 != status)
            return status;

        if (!res.node->is_dir())
            return -ENOTDIR;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.node);

        if (dir->mounted_root)
            return -EEXIST;

        dir->mounted_root = fs->GetRoot();
        this->block_devices[fs->GetBlkId()] = fs;

        return 0;
    }

    // mount fs at path (target must exist and be directory)
    int QFS::unmount(const fs::path &path)
    {
        Resolved res{};
        int status = resolve(path, res);

        if (0 != status)
            return status;

        auto target_blkdev = this->block_devices.find(res.mountpoint->GetBlkId());
        if (target_blkdev == this->block_devices.end())
            // not mounted
            return -EINVAL;

        dir_ptr dir = std::static_pointer_cast<Directory>(res.parent);

        if (nullptr == dir->mounted_root)
            // mounted but rootdir disappeared O.o
            return -EINVAL;

        dir->mounted_root = nullptr;
        this->block_devices.erase(res.mountpoint->GetBlkId());

        return 0;
    }

};
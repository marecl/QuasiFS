#include "include/quasi_errno.h"
#include "include/quasifs_types.h"

#include "include/quasifs_inode_directory.h"
#include "include/quasifs_inode_regularfile.h"
#include "include/quasifs_inode_symlink.h"
#include "include/quasifs_partition.h"
#include "include/quasifs.h"

#include "../log.h"

namespace QuasiFS
{

    int QFS::Open(const fs::path &path, int flags, quasi_mode_t mode)
    {
        Resolved r{};
        int status = this->Resolve(path, r);

        partition_ptr part = r.mountpoint;
        dir_ptr parent_node = r.parent;
        inode_ptr target = r.node;

        // prepare file descriptor
        auto next_free_handle = this->GetFreeHandleNo();
        fd_handle_ptr handle = File::Create();

        handle->node = target;
        handle->read = flags & O_WRONLY ? false : true;
        handle->write = flags & O_WRONLY ? true : (flags & O_RDWR);

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (!part->GetHostPath(host_path_target, r.local_path))
            {
                _errno = QUASI_EACCES;
                return -1;
            }
            int host_fd = this->hio_driver.Open(host_path_target.c_str(), flags, mode);
            if (-1 == host_fd)
                // hosts operation must succeed in order to continue
                return -1;
            handle->host_fd = host_fd;
            // handle->
        }
        else
        {
            bool exists = (-QUASI_ENOENT != status) && target;

            // check for RO!

            if (exists && (flags & O_EXCL) && (flags & O_CREAT))
                return -QUASI_EEXIST;

            if (!exists)
            {
                if ((flags & O_CREAT) == 0)
                    return -QUASI_ENOENT;

                part->touch(parent_node, r.leaf);
                target = parent_node->lookup(r.leaf);
                if (nullptr == target)
                    // touch failed in target directory, issue with resolve() most likely
                    return -QUASI_EFAULT;
            }

            if (flags & O_TRUNC)
            {
                if (target->is_file())
                    std::static_pointer_cast<RegularFile>(target)->truncate(0);
                else if (target->is_dir())
                    return -QUASI_EISDIR;
                else
                    return -QUASI_EINVAL;
            }

            // if exists and is a directory, can't be opened with any kind of write
            if (exists && (target->is_dir() || (flags & O_DIRECTORY)) && (flags & (O_TRUNC | O_RDWR | O_WRONLY)))
                return -QUASI_EISDIR;

            if ((flags & O_DIRECTORY) && !target->is_dir())
                // opening dirs isn't supported yet
                return -QUASI_ENOTDIR;

            if (flags & (O_APPEND | O_NOFOLLOW | O_PATH /* | O_TMPFILE */))
            {
                // O_TMPFILE expansion includes O_DIRECTORY
                // not implemented
                return -QUASI_EINVAL;
            }

            if (flags & (O_NONBLOCK | O_SYNC | O_ASYNC | O_CLOEXEC | O_DIRECT | O_DSYNC | O_LARGEFILE | O_NOATIME | O_NOCTTY))
            {
                // unused, not affecting file manip per-se
            }
        }

        handle->node = target;

        this->open_fd[next_free_handle] = handle;
        return next_free_handle;
    }

    int QFS::Creat(const fs::path &path, quasi_mode_t mode)
    {
        Resolved r{};
        int status = this->Resolve(path.parent_path(), r);

        // TODO: catch what if target exists and is not a file!

        if (nullptr == r.node)
        {
            // parent node must exist, file does not
            _errno = -status;
            return -1;
        }

        if (r.mountpoint->IsHostMounted())
        {
            fs::path host_path_target{};
            if (!r.mountpoint->GetHostPath(host_path_target, r.local_path / path.filename()))
            {
                _errno = QUASI_EACCES;
                return -1;
            }

            if (int status = this->hio_driver.Close(this->hio_driver.Creat(host_path_target, mode)); 0 != status)
                // hosts operation must succeed in order to continue
                return status;
        }

        this->vio_driver.set(r);
        status = this->vio_driver.Creat(path, mode);
        this->vio_driver.clear();

        return status;
    };

    int QFS::Close(int fd)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -QUASI_EBADF;

        fd_handle_ptr handle = this->open_fd.at(fd);
        if (nullptr == handle)
        {
            _errno = QUASI_EBADF;
            return -1;
        }

        if (handle->host_fd >= 0)
        {
            if (0 != this->hio_driver.Close(handle->host_fd))
                return -1;
        }

        this->open_fd.at(fd) = nullptr;
        return 0;
    }

    int QFS::LinkSymbolic(const fs::path &src, const fs::path &dst)
    { // stub
        return -1;
    }

    int QFS::Link(const fs::path &src, const fs::path &dst)
    {
        Resolved sos{};
        Resolved tar{};
        int status_what = Resolve(src, sos);
        int status_where = Resolve(dst, tar);

        if (0 != status_what)
            return status_what;
        if (0 == status_where)
            return -QUASI_EEXIST;

        // cross-partition linking is not supported
        if (sos.mountpoint != tar.mountpoint)
        {
            _errno = QUASI_EXDEV;
            return -1;
        }

        inode_ptr sos_inode = sos.node;
        return tar.parent->link(tar.leaf, sos_inode);
    }

    int QFS::Unlink(const fs::path &path)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        partition_ptr part = res.mountpoint;
        dir_ptr dir = res.parent;
        // inode_ptr target = res.node;

        return part->unlink(dir, res.leaf);
    }

    int QFS::Flush(const int fd)
    { // not our job, delegate to host if applicable
        // TODO: implement further
        return this->hio_driver.Flush(fd);
    }

    int QFS::FSync(const int fd)
    { // not our job, delegate to host if applicable
        // TODO: implement further
        return this->hio_driver.FSync(fd);
    };

    int QFS::Truncate(const fs::path &path, quasi_size_t length)
    {
        Resolved r{};
        int status = Resolve(path, r);
        if (0 != status)
            return status;
        if (r.node->is_file())
            return std::static_pointer_cast<RegularFile>(r.node)->truncate(length);
        return -QUASI_EINVAL;
    }

    int QFS::FTruncate(const int fd, quasi_size_t length)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -QUASI_EBADF;
        fd_handle_ptr handle = this->open_fd.at(fd);
        inode_ptr target = handle->node;

        if (target->is_file())
            return std::static_pointer_cast<RegularFile>(target)->truncate(length);
        return -QUASI_EBADF;
    }

    quasi_off_t QFS::LSeek(const int fd, quasi_off_t offset, SeekOrigin origin)
    { // stub
        return -QUASI_EINVAL;
    };

    quasi_ssize_t QFS::Tell(int fd)
    { // stub
        return -QUASI_EINVAL;
    };

    quasi_ssize_t QFS::Write(const int fd, const void *buf, quasi_size_t count)
    {
        return PWrite(fd, buf, count, 0);
    }

    quasi_ssize_t QFS::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    { // stub
        return -QUASI_EINVAL;
    };

    // quasi_ssize_t QFS::pwrite(int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    // {
    //     if (0 > fd || fd >= this->open_fd.size())
    //         return -QUASI_EBADF;
    //     fd_handle_ptr handle = this->open_fd.at(fd);
    //     if (!handle->write)
    //         return -QUASI_EBADF;

    //     inode_ptr target = handle->node;
    //     if (target->is_file())
    //         return std::static_pointer_cast<RegularFile>(target)->write(offset, buf, count);
    //     // TODO: remaining types
    //     return -QUASI_EBADF;
    // }

    quasi_ssize_t QFS::Read(const int fd, void *buf, quasi_size_t count)
    {
        return PRead(fd, buf, count, 0);
    }

    quasi_ssize_t QFS::PRead(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    { // stub
        return -QUASI_EINVAL;
    };

    // quasi_ssize_t QFS::pread(int fd, void *buf, quasi_size_t count, quasi_off_t offset)
    // {
    //     if (0 > fd || fd >= this->open_fd.size())
    //         return -QUASI_EBADF;
    //     fd_handle_ptr handle = this->open_fd.at(fd);
    //     if (!handle->read)
    //         return -QUASI_EBADF;

    //     inode_ptr target = handle->node;
    //     if (target->is_file())
    //         return std::static_pointer_cast<RegularFile>(target)->read(offset, buf, count);
    //     // TODO: remaining types
    //     return -QUASI_EBADF;
    // }

    int QFS::MKDir(const fs::path &path, quasi_mode_t mode)
    {
        Resolved r{};
        int status = this->Resolve(path, r);

        // catch what if target exists and is not a file!
        if (-QUASI_ENOENT != status)
            // parent node must exist
            return status;

        if (r.mountpoint->IsHostMounted())
        {
            fs::path host_path_target{};
            if (!r.mountpoint->GetHostPath(host_path_target, r.local_path))
            {
                _errno = QUASI_EACCES;
                return -1;
            }

            if (int status = this->hio_driver.MKDir(host_path_target, mode); 0 != status)
                // hosts operation must succeed in order to continue
                return status;
        }

        this->vio_driver.set(r);
        status = this->vio_driver.MKDir(path, mode);
        this->vio_driver.clear();

        return status;
    }

    int QFS::RMDir(const fs::path &path)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        dir_ptr parent = res.parent;

        if (parent->mounted_root)
            return -QUASI_EBUSY;

        if (0 == parent->unlink(res.leaf))
            return res.mountpoint->rmInode(res.node);
        return 0;
    }

    int QFS::Stat(const fs::path &path, quasi_stat_t *stat)
    { // stub
        return -QUASI_EINVAL;
    }

    int QFS::FStat(const int fd, quasi_stat_t *statbuf)
    { // stub
        return -QUASI_EINVAL;
    }
}
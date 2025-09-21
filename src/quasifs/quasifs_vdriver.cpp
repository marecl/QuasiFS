#include "quasi_errno.h"
#include "include/quasifs_types.h"

#include "include/quasifs_inode_directory.h"
#include "include/quasifs_inode_regularfile.h"
#include "include/quasifs_inode_symlink.h"
#include "include/quasifs_partition.h"
#include "include/quasifs.h"

namespace QuasiFS
{

    int QFS::Open(const fs::path &path, int flags, int mode)
    {
        Resolved r{};
        int status = this->resolve(path, r);
        inode_ptr target = r.node;
        bool exists = (-QUASI_ENOENT != status) && target;

        // check for RO!

        if (exists && (flags & O_EXCL) && (flags & O_CREAT))
            return -QUASI_EEXIST;

        if (!exists)
        {
            if ((flags & O_CREAT) == 0)
                return -QUASI_ENOENT;

            this->touch(path);
            target = r.parent->lookup(r.leaf);
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

        auto next_free_handle = this->GetFreeHandleNo();
        fd_handle_ptr handle = File::Create();

        handle->node = target;
        handle->read = flags & O_WRONLY ? false : true;
        handle->write = flags & O_WRONLY ? true : (flags & O_RDWR);

        this->open_fd[next_free_handle] = handle;
        return next_free_handle;
    }

    int QFS::Close(int fd)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -QUASI_EBADF;

        this->open_fd.at(fd) = nullptr;
        return 0;
    }

    int QFS::Link(const fs::path &src, const fs::path &dst)
    {
        Resolved sos{};
        Resolved tar{};
        int status_what = resolve(src, sos);
        int status_where = resolve(dst, tar);

        if (0 != status_what)
            return status_what;
        if (0 == status_where)
            return -QUASI_EEXIST;

        // cross-partition linking is not supported
        if (sos.mountpoint != tar.mountpoint)
            return -QUASI_EXDEV;

        inode_ptr sos_inode = sos.node;
        return tar.parent->link(tar.leaf, sos_inode);
    }

    int QFS::Unlink(const fs::path &path)
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

    int QFS::Flush(const int fd)
    { // stub
        return -QUASI_EINVAL;
    }
    int QFS::FSync(const int fd)
    { // stub
        return -QUASI_EINVAL;
    };

    int QFS::Truncate(const fs::path &path, size_t length)
    {
        Resolved r{};
        int status = resolve(path, r);
        if (0 != status)
            return status;
        if (r.node->is_file())
            return std::static_pointer_cast<RegularFile>(r.node)->truncate(length);
        return -QUASI_EINVAL;
    }

    int QFS::FTruncate(int fd, size_t length)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -QUASI_EBADF;
        fd_handle_ptr handle = this->open_fd.at(fd);
        inode_ptr target = handle->node;

        if (target->is_file())
            return std::static_pointer_cast<RegularFile>(target)->truncate(length);
        return -QUASI_EBADF;
    }

    off_t QFS::LSeek(const int fd, off_t offset, SeekOrigin origin)
    { // stub
        return -QUASI_EINVAL;
    };

    ssize_t QFS::Tell(int fd)
    { // stub
        return -QUASI_EINVAL;
    };

    ssize_t QFS::Write(int fd, const void *buf, size_t count)
    {
        return PWrite(fd, buf, count, 0);
    }

    ssize_t QFS::PWrite(int fd, const void *buf, size_t count, off_t offset)
    { // stub
        return -QUASI_EINVAL;
    };
    // ssize_t QFS::pwrite(int fd, const void *buf, size_t count, off_t offset)
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

    ssize_t QFS::Read(int fd, void *buf, size_t count)
    {
        return PRead(fd, buf, count, 0);
    }

    ssize_t QFS::PRead(int fd, const void *buf, size_t count, off_t offset)
    { // stub
        return -QUASI_EINVAL;
    };
    // ssize_t QFS::pread(int fd, void *buf, size_t count, off_t offset)
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

    int QFS::MKDir(const fs::path &path, int mode)
    {
        fs::path base = path.parent_path();
        std::string fname = path.filename();

        return this->mkdir(base, fname);
    }

    int QFS::RMDir(const fs::path &path)
    { // stub
        return -QUASI_EINVAL;
    };

    int QFS::Stat(fs::path &path, ::QuasiFS::Stat *stat)
    { // stub
        return -QUASI_EINVAL;
    }
    int QFS::FStat(int fd, ::QuasiFS::Stat *statbuf)
    { // stub
        return -QUASI_EINVAL;
    }
}
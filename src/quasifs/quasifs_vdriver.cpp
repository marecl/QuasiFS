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

    int QFS::Open(const fs::path &path, int flags, mode_t mode)
    {
        Resolved r{};
        int status = this->Resolve(path, r);
        inode_ptr target = r.node;
        bool exists = (-QUASI_ENOENT != status) && target;

        // check for RO!

        if (exists && (flags & O_EXCL) && (flags & O_CREAT))
            return -QUASI_EEXIST;

        if (!exists)
        {
            if ((flags & O_CREAT) == 0)
                return -QUASI_ENOENT;

            this->Touch(path);
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

    int QFS::Creat(const fs::path &path, mode_t mode)
    {
        Resolved r{};
        fs::path dirtree = path.parent_path();
        fs::path leaf = path.filename();
        int status = this->Resolve(path, r);

        // catch what if target exists and is not a file!
        if (-QUASI_ENOENT != status)
            // parent node must exist
            return status;

        dir_ptr parent_node = r.parent;
        fs::path host_path_target = GetHostPathByInode(parent_node);

        if (!host_path_target.empty())
        {
            Log("Resolving local {} to hosts {}", path.string(), (host_path_target / leaf).string());

            status = driver.Creat(host_path_target / leaf, mode);

            if (status != 0)
                // hosts operation must succeed in order to continue
                return status;
        }

        file_ptr new_file = RegularFile::Create();
        status = r.mountpoint->touch(parent_node, leaf, new_file);

        if (0 != status)
            // if host succeeded, this must too
            return status;

        if (!host_path_target.empty())
            this->host_files[new_file] = host_path_target;

        return 0;
    };

    int QFS::Close(int fd)
    {
        if (0 > fd || fd >= this->open_fd.size())
            return -QUASI_EBADF;

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
            return -QUASI_EXDEV;

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
        int status = Resolve(path, r);
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
        Resolved r{};
        fs::path dirtree = path.parent_path();
        fs::path leaf = path.filename();
        int status = this->Resolve(path, r);

        // catch what if target exists and is not a file!
        if (-QUASI_ENOENT != status)
            // parent node must exist
            return status;

        dir_ptr parent_node = r.parent;
        fs::path host_path_parent = GetHostPathByInode(parent_node);

        if (!host_path_parent.empty())
        {
            Log("Resolving local {} to hosts {}", path.string(), (host_path_parent / leaf).string());
            status = driver.MKDir(host_path_parent / leaf);
            if (0 != status)
                return status;
        }

        fs::path base = path.parent_path();
        dir_ptr new_dir = Directory::Create();

        status = this->MKDir(base, leaf, new_dir);

        if (0 != status)
            return status;

        if (!host_path_parent.empty())
            this->host_files[new_dir] = host_path_parent / leaf;

        return 0;
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

    int QFS::Stat(fs::path &path, quasi_stat_t *stat)
    { // stub
        return -QUASI_EINVAL;
    }

    int QFS::FStat(int fd, quasi_stat_t *statbuf)
    { // stub
        return -QUASI_EINVAL;
    }
}
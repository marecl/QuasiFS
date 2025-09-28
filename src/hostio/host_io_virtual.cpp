#include <filesystem>

#include <sys/types.h>
#include <sys/fcntl.h>

#include "../../quasifs/include/quasi_errno.h"
#include "../../quasifs/include/quasifs_types.h"
#include "../../quasifs/include/quasifs_inode_directory.h"
#include "../../quasifs/include/quasifs_inode_regularfile.h"
#include "../../quasifs/include/quasifs_inode_symlink.h"
#include "../../quasifs/include/quasifs_partition.h"

#include "host_io_base.h"
#include "host_io_virtual.h"

namespace HostIODriver
{

    HostIO_Virtual::HostIO_Virtual() = default;
    HostIO_Virtual::~HostIO_Virtual() = default;

    int HostIO_Virtual::Open(const fs::path &path, int flags, quasi_mode_t mode)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;



        inode_ptr target = this->res->node;
        dir_ptr parent = this->res->parent;
        partition_ptr part = this->res->mountpoint;

        bool exists = this->res->node != nullptr;

        // check for RO!
        std::string qweqwe =this->res->local_path;

        if (exists && (flags & O_EXCL) && (flags & O_CREAT))
            return -QUASI_EEXIST;

        if (!exists)
        {
            if ((flags & O_CREAT) == 0)
                return -QUASI_ENOENT;

            part->touch(parent, this->res->leaf);
            target = parent->lookup(this->res->leaf);
            if (nullptr == target)
                // touch failed in target directory, issue with resolve() most likely
                return -QUASI_EFAULT;

            this->res->node = target;
        }

        // at this point target should exist

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

        return 0;
    }

    int HostIO_Virtual::Creat(const fs::path &path, quasi_mode_t mode)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;
        if (!this->res->node->is_dir())
            return -QUASI_ENOTDIR;

        dir_ptr parent = std::static_pointer_cast<Directory>(this->res->node);
        file_ptr new_file = RegularFile::Create();
        return this->res->mountpoint->touch(parent, path.filename(), new_file);
    }

    // unused, QFS handles everything
    // int HostIO_Virtual::Close(const int fd)
    // {
    //     return 0;
    // }

    int HostIO_Virtual::LinkSymbolic(const fs::path &src, const fs::path &dst)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;

        symlink_ptr sym = Symlink::Create(src);
        // symlink counter is never increased
        sym->st.st_nlink = 1;

        this->res->mountpoint->IndexInode(sym);
        return this->res->parent->link(dst.filename(), sym);
    }

    int HostIO_Virtual::Link(const fs::path &src, const fs::path &dst)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;

        partition_ptr part = this->res->mountpoint;
        inode_ptr src_node = this->res->node;

        Resolved dst_res{};
        fs::path dst_path = dst.parent_path();
        std::string dst_name = dst.filename();

        if (int res_status = part->Resolve(dst_path, dst_res); res_status < 0)
            return res_status;

        if (!dst_res.node->is_dir())
            return -QUASI_ENOTDIR;

        dir_ptr dst_parent = std::static_pointer_cast<Directory>(dst_res.node);
        return part->link(src_node, dst_parent, dst_name);
    }

    int HostIO_Virtual::Unlink(const fs::path &path)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;

        if (nullptr == this->res->node)
            return -QUASI_ENOENT;

        if (this->res->node->is_dir())
            return -QUASI_EISDIR;

        // EINVAL on . as last element

        dir_ptr parent = this->res->parent;
        inode_ptr node = this->res->node;

        // probably redundant
        if (inode_ptr node_tmp = parent->lookup(this->res->leaf); node_tmp != node)
            LogError("Resolved to different inode (0x{:x}) than parent holds (0x{:x})", node->st.st_ino, node_tmp ? node_tmp->st.st_ino : -1);

        if (int unlink_status = this->res->parent->unlink(this->res->leaf); unlink_status != 0)
            return unlink_status;

        return res->mountpoint->rmInode(res->node);
    }

    // int HostIO_Virtual::Flush(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::FSync(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::Truncate(const fs::path &path, quasi_size_t size)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::FTruncate(const int fd, quasi_size_t size)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_off_t HostIO_Virtual::LSeek(const int fd, quasi_off_t offset, QuasiFS::SeekOrigin origin)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::Tell(const int fd)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::Write(const int fd, const void *buf, quasi_size_t count)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::Read(const int fd, void *buf, quasi_size_t count)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // quasi_ssize_t HostIO_Virtual::PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    int HostIO_Virtual::MKDir(const fs::path &path, quasi_mode_t mode)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;

        dir_ptr new_dir = Directory::Create();
        return this->res->mountpoint->mkdir(this->res->parent, this->res->leaf, new_dir);
    }

    int HostIO_Virtual::RMDir(const fs::path &path)
    {
        if (nullptr == this->res)
            return -QUASI_EINVAL;

        // EINVAL on . as last element

        // don't remove partition root ;___;
        dir_ptr parent = this->res->parent;

        if (parent->mounted_root)
            return -QUASI_EBUSY;

        if (int unlink_status = parent->unlink(this->res->leaf); unlink_status != 0)
            return unlink_status;

        auto target_nlink = res->node->st.st_nlink;
        if (target_nlink != 0)
        {
            LogError("RMDir'd directory nlink is not 0!", "(is ", target_nlink, ")");
            return -QUASI_ENOTEMPTY;
        }

        return res->mountpoint->rmInode(res->node);
    }

    // int HostIO_Virtual::Stat(const fs::path &path, QuasiFS::quasi_stat_t *statbuf)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }

    // int HostIO_Virtual::FStat(const int fd, QuasiFS::quasi_stat_t *statbuf)
    // {
    //     if (nullptr == this->res)
    //         return -1;
    //     return -1;
    // }
}

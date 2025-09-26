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
        // resolve for parent dir to avoid treating ENOENT as missing just the end file
        int resolve_status = this->Resolve(path.parent_path(), r);

        if (0 != resolve_status)
            return resolve_status;

        if (!r.node->is_dir())
            // dunno
            return -QUASI_EINVAL;

        partition_ptr part = r.mountpoint;
        dir_ptr parent_node = std::static_pointer_cast<Directory>(r.node);

        // update context fot vdriver
        r.parent = parent_node;
        r.node = parent_node->lookup(path.filename());
        r.leaf = path.filename();

        // prepare file descriptor

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, r.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Open(host_path_target / path.filename(), flags, mode); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.set(&r);
        vio_status = this->vio_driver.Open(path, flags, mode);
        this->vio_driver.clear();

        if (int tmp_hio_status = hio_status >= 0 ? 0 : hio_status; host_used && (tmp_hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        if (vio_status < 0)
            return vio_status;

        fd_handle_ptr handle = File::Create();
        // nasty hack, but: of it existed, no change
        // if it didn't, VIO will update this member
        handle->node = r.node;
        // virtual fd is stored in open_fd map
        handle->host_fd = hio_status;
        handle->read = !(flags & O_WRONLY);
        handle->write = flags & O_WRONLY ? true : (flags & O_RDWR);
        auto next_free_handle = this->GetFreeHandleNo();
        this->open_fd[next_free_handle] = handle;
        return next_free_handle;
    }

    int QFS::Creat(const fs::path &path, quasi_mode_t mode)
    {
        return Open(path, O_CREAT | O_WRONLY | O_TRUNC, mode);
    };

    int QFS::Close(int fd)
    {
        if (fd < 0 || fd >= this->open_fd.size())
            return -QUASI_EBADF;

        fd_handle_ptr handle = this->open_fd.at(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        // if it fails, it fails
        if (int hio_status = this->hio_driver.Close(handle->host_fd); hio_status < 0)
            return hio_status;

        // no further action is required

        auto handle_inode_links = &(handle->node->st.st_nlink);

        this->open_fd.at(fd) = nullptr;
        return 0;
    }

    // int QFS::LinkSymbolic(const fs::path &src, const fs::path &dst)
    // { // stub
    //     return -1;
    // }

    int QFS::Link(const fs::path &src, const fs::path &dst)
    {
        Resolved src_res{};
        Resolved dst_res{};
        int status_what = Resolve(src, src_res);
        int status_where = Resolve(dst, dst_res);

        if (0 != status_what)
            return status_what;
        if (0 == status_where)
            return -QUASI_EEXIST;

        // cross-partition linking is not supported
        if (src_res.mountpoint != dst_res.mountpoint)
            return -QUASI_EXDEV;

        partition_ptr part = src_res.mountpoint;
        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_src{};
            fs::path host_path_dst{};

            if (int hostpath_status = part->GetHostPath(host_path_src, src_res.local_path); hostpath_status != 0)
                return hostpath_status;
            if (int hostpath_status = part->GetHostPath(host_path_dst, dst_res.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Link(host_path_src, host_path_dst); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.set(&src_res);
        vio_status = this->vio_driver.Link(src_res.local_path, dst_res.local_path);
        this->vio_driver.clear();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    int QFS::Unlink(const fs::path &path)
    {
        Resolved r{};
        int resolve_status = Resolve(path, r);

        if (nullptr == r.node || resolve_status < 0)
        {
            // parent node must exist, file does not
            return resolve_status;
        }

        partition_ptr part = r.mountpoint;
        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, r.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Unlink(host_path_target); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.set(&r);
        vio_status = this->vio_driver.Unlink(r.local_path);
        this->vio_driver.clear();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    // int QFS::Flush(const int fd)
    // { // not our job, delegate to host if applicable
    //     // TODO: implement further
    //     return this->hio_driver.Flush(fd);
    // }

    // int QFS::FSync(const int fd)
    // { // not our job, delegate to host if applicable
    //     // TODO: implement further
    //     return this->hio_driver.FSync(fd);
    // };

    // int QFS::Truncate(const fs::path &path, quasi_size_t length)
    // {
    //     Resolved r{};
    //     int status = Resolve(path, r);
    //     if (0 != status)
    //         return status;
    //     if (r.node->is_file())
    //         return std::static_pointer_cast<RegularFile>(r.node)->truncate(length);
    //     return -QUASI_EINVAL;
    // }

    // int QFS::FTruncate(const int fd, quasi_size_t length)
    // {
    //     if (0 > fd || fd >= this->open_fd.size())
    //         return -QUASI_EBADF;
    //     fd_handle_ptr handle = this->open_fd.at(fd);
    //     inode_ptr target = handle->node;

    //     if (target->is_file())
    //         return std::static_pointer_cast<RegularFile>(target)->truncate(length);
    //     return -QUASI_EBADF;
    // }

    // quasi_off_t QFS::LSeek(const int fd, quasi_off_t offset, SeekOrigin origin)
    // { // stub
    //     return -QUASI_EINVAL;
    // };

    // quasi_ssize_t QFS::Tell(int fd)
    // { // stub
    //     return -QUASI_EINVAL;
    // };

    // quasi_ssize_t QFS::Write(const int fd, const void *buf, quasi_size_t count)
    // {
    //     return PWrite(fd, buf, count, 0);
    // }

    // quasi_ssize_t QFS::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    // { // stub
    //     return -QUASI_EINVAL;
    // };

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

    // quasi_ssize_t QFS::Read(const int fd, void *buf, quasi_size_t count)
    // {
    //     return PRead(fd, buf, count, 0);
    // }

    // quasi_ssize_t QFS::PRead(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    // { // stub
    //     return -QUASI_EINVAL;
    // };

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
        int resolve_status = this->Resolve(path, r);

        if (0 == resolve_status)
            return -QUASI_EEXIST;

        // catch what if target exists and is not a file!
        if (-QUASI_ENOENT != resolve_status)
            // parent node must exist
            return resolve_status;

        partition_ptr part = r.mountpoint;
        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, r.local_path); 0 != hostpath_status)
                return hostpath_status;

            if (hio_status = this->hio_driver.MKDir(host_path_target, mode); 0 != hio_status)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.set(&r);
        vio_status = this->vio_driver.MKDir(path, mode);
        this->vio_driver.clear();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    int QFS::RMDir(const fs::path &path)
    {
        Resolved r{};
        int status = Resolve(path, r);

        if (0 != status)
            return status;

        partition_ptr part = r.mountpoint;
        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, r.local_path); 0 != hostpath_status)
                return hostpath_status;

            if (hio_status = this->hio_driver.RMDir(host_path_target); 0 != hio_status)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.set(&r);
        status = this->vio_driver.RMDir(path);
        this->vio_driver.clear();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return status;
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
#include <cstring>

#include <sys/fcntl.h>

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
        Resolved res{};
        // resolve for parent dir to avoid treating ENOENT as missing just the end file
        int resolve_status = this->Resolve(path, res);

        // enoent on last element in the path is good
        if (-QUASI_ENOENT == resolve_status)
        {
            if (nullptr == res.parent)
                return -QUASI_ENOENT;
        }
        else if (0 != resolve_status)
            return resolve_status;

        partition_ptr part =res.mountpoint;
        dir_ptr parent_node = std::static_pointer_cast<Directory>(res.parent);

        bool request_read = !(flags & O_WRONLY) | O_RDWR;
        bool request_write = flags & (O_WRONLY | O_RDWR);
        bool request_append = flags & O_APPEND;

        //
        // Prioritize QFS permissions
        // Host is (usually) more lenient
        //

        if ((request_write || request_append) && IsPartitionRO(part))
            return -QUASI_EROFS;

        // if it doesn't exist, check the parent
        inode_ptr checked_node = nullptr == res.node ? parent_node : res.node;

        if ((request_read && !checked_node->CanRead()) ||
            (request_write && !checked_node->CanWrite()))
            return -QUASI_EACCES;

        //
        // Proceed
        //

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Open(host_path_target, flags, mode); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(&res,host_used, nullptr);
        vio_status = this->vio_driver.Open(res.local_path, flags, mode);
        this->vio_driver.ClearCtx();

        if (int tmp_hio_status = hio_status >= 0 ? 0 : hio_status; host_used && (tmp_hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        if (vio_status < 0)
            return vio_status;

        fd_handle_ptr handle = File::Create();
        // nasty hack, but: of it existed, no change
        // if it didn't, VIO will update this member
        handle->node = res.node;
        // virtual fd is stored in open_fd map
        handle->host_fd = host_used ? hio_status : -1;
        handle->read = request_read;
        handle->write = request_write;
        handle->append = request_append;
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
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        // if it fails, it fails
        if (int hio_status = this->hio_driver.Close(handle->host_fd); hio_status < 0)
            return hio_status;

        // no further action is required, this is pro-forma
        this->vio_driver.Close(fd);

        // if it's the last entry, remove it to avoid blowing up fd table
        // not really helping with fragmentation, but may save resources on burst opens

        if (fd < (this->open_fd.size() - 1))
        {
            this->open_fd.at(fd) = nullptr;
            return 0;
        }

        this->open_fd.pop_back();
        return 0;
    }

    int QFS::LinkSymbolic(const fs::path &src, const fs::path &dst)
    {
        Resolved src_res{};
        Resolved dst_res{};
        int status_what = Resolve(src, src_res);
        int status_where = Resolve(dst, dst_res);

        // source may not exist and can point wherever it wants, so we skip checks for it

        if (0 == status_where)
            return -QUASI_EEXIST;
        // destination parent directory must exist though
        if (0 != status_where && nullptr == dst_res.parent)
            return -QUASI_ENOENT;

        partition_ptr src_part = src_res.mountpoint;
        partition_ptr dst_part = dst_res.mountpoint;

        if (IsPartitionRO(dst_part))
            return -QUASI_EROFS;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        // for this to work, both files must be on host partition
        // mixed source/destination will need a bit more effort
        if (src_part->IsHostMounted() && dst_part->IsHostMounted())
        {
            // if target partition doesn't exist or is not mounted, we can't resolve host path
            if (nullptr == src_part)
                return -QUASI_ENOENT;

            fs::path host_path_src{};
            fs::path host_path_dst{};

            if (int hostpath_status = src_part->GetHostPath(host_path_src, src_res.local_path); hostpath_status != 0)
                return hostpath_status;
            if (int hostpath_status = dst_part->GetHostPath(host_path_dst, dst_res.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.LinkSymbolic(host_path_src, host_path_dst); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }
        else if (dst_part->IsHostMounted() ^ src_part->IsHostMounted())
        {
            LogError("Symlinks can be only created if both source and destination are host-bound");
            return -QUASI_ENOSYS;
        }

        this->vio_driver.SetCtx(&dst_res, host_used, nullptr);
        // src stays 1:1
        vio_status = this->vio_driver.LinkSymbolic(src, dst_res.local_path);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

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

        partition_ptr src_part = src_res.mountpoint;
        partition_ptr dst_part = dst_res.mountpoint;

        if (IsPartitionRO(dst_part))
            return -QUASI_EROFS;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (dst_part->IsHostMounted() && src_part->IsHostMounted())
        {
            fs::path host_path_src{};
            fs::path host_path_dst{};

            if (int hostpath_status = src_part->GetHostPath(host_path_src, src_res.local_path); hostpath_status != 0)
                return hostpath_status;
            if (int hostpath_status = dst_part->GetHostPath(host_path_dst, dst_res.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Link(host_path_src, host_path_dst); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }
        else if (dst_part->IsHostMounted() ^ src_part->IsHostMounted())
        {
            LogError("Links can be only created if both source and destination are host-bound");
            return -QUASI_ENOSYS;
        }

        this->vio_driver.SetCtx(&src_res, host_used, nullptr);
        vio_status = this->vio_driver.Link(src_res.local_path, dst_res.local_path);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    int QFS::Unlink(const fs::path &path)
    {
        Resolved res{};
        int resolve_status;

        // symlinks mess this whole thing up, so we need to resolve parent and leaf independently

        fs::path parent_path = path.parent_path();
        fs::path leaf = path.filename();

        // parent, must pass
        resolve_status = Resolve(parent_path, res);
        if (resolve_status != 0)
            return resolve_status;

        if (!res.node->is_dir())
            return -QUASI_ENOTDIR;

        partition_ptr part =res.mountpoint;
        if (IsPartitionRO(part))
            return -QUASI_EROFS;

        dir_ptr parent = std::static_pointer_cast<Directory>(res.node);
        inode_ptr target = parent->lookup(leaf);

        if (nullptr == target)
            return -QUASI_ENOENT;

        // fix up resolve result for VIO
        res.parent = parent;
        res.node = target;
        res.leaf = leaf;
        res.local_path /= leaf;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Unlink(host_path_target); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(&res,host_used, nullptr);
        vio_status = this->vio_driver.Unlink(res.local_path);
        this->vio_driver.ClearCtx();

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

    int QFS::Truncate(const fs::path &path, quasi_size_t length)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        partition_ptr part =res.mountpoint;
        if (IsPartitionRO(part))
            return -QUASI_EROFS;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target;
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path); 0 != hostpath_status)
                return hostpath_status;
            if (hio_status = this->hio_driver.Truncate(host_path_target, length); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(&res,host_used, nullptr);
        vio_status = this->vio_driver.Truncate(res.local_path, length);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    int QFS::FTruncate(const int fd, quasi_size_t length)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        if (!handle->write)
            return -QUASI_EBADF;

        // EROFS is guarded by Open()

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            if (hio_status = this->hio_driver.FTruncate(host_fd, length); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.FTruncate(fd, length);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    quasi_off_t QFS::LSeek(const int fd, quasi_off_t offset, SeekOrigin origin)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            if (hio_status = this->hio_driver.LSeek(host_fd, offset, origin); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.LSeek(fd, offset, origin);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    };

    quasi_ssize_t QFS::Tell(int fd)
    {
        return LSeek(fd, 0, SeekOrigin::CURRENT);
    };

    quasi_ssize_t QFS::Write(const int fd, const void *buf, quasi_size_t count)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        if (!handle->write)
            return -QUASI_EBADF;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            if (hio_status = this->hio_driver.Write(host_fd, buf, count); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.Write(fd, buf, count);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    quasi_ssize_t QFS::PWrite(const int fd, const void *buf, quasi_size_t count, quasi_off_t offset)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        if (!handle->write)
            return -QUASI_EBADF;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            if (hio_status = this->hio_driver.PWrite(host_fd, buf, count, offset); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.PWrite(fd, buf, count, offset);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    };

    quasi_ssize_t QFS::Read(const int fd, void *buf, quasi_size_t count)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        if (!handle->read)
            return -QUASI_EBADF;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            if (hio_status = this->hio_driver.Read(host_fd, buf, count); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.Read(fd, buf, count);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    quasi_ssize_t QFS::PRead(const int fd, void *buf, quasi_size_t count, quasi_off_t offset)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        if (!handle->read)
            return -QUASI_EBADF;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            if (hio_status = this->hio_driver.PRead(host_fd, buf, count, offset); hio_status < 0)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.PRead(fd, buf, count, offset);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    };

    int QFS::MKDir(const fs::path &path, quasi_mode_t mode)
    {
        Resolved res{};
        int resolve_status = this->Resolve(path, res);

        if (0 == resolve_status)
            return -QUASI_EEXIST;

        if (-QUASI_ENOENT == resolve_status)
        {
            if (nullptr == res.parent)
                return -QUASI_ENOENT;
        }
        else if (0 != resolve_status)
            return resolve_status;

        partition_ptr part =res.mountpoint;

        if (IsPartitionRO(part))
            return -QUASI_EROFS;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path); 0 != hostpath_status)
                return hostpath_status;

            if (hio_status = this->hio_driver.MKDir(host_path_target, mode); 0 != hio_status)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(&res,host_used, nullptr);
        vio_status = this->vio_driver.MKDir(res.local_path, mode);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    int QFS::RMDir(const fs::path &path)
    {
        Resolved res{};
        int status = Resolve(path, res);

        if (0 != status)
            return status;

        partition_ptr part =res.mountpoint;
        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path); 0 != hostpath_status)
                return hostpath_status;

            if (hio_status = this->hio_driver.RMDir(host_path_target); 0 != hio_status)
                // hosts operation must succeed in order to continue
                return hio_status;
            host_used = true;
        }

        this->vio_driver.SetCtx(&res,host_used, nullptr);
        status = this->vio_driver.RMDir(res.local_path);
        this->vio_driver.ClearCtx();

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return status;
    }

    int QFS::Stat(const fs::path &path, quasi_stat_t *statbuf)
    {
        Resolved res{};
        int resolve_status = Resolve(path, res);

        if (nullptr == res.node || resolve_status < 0)
        {
            // parent node must exist, file does not
            return resolve_status;
        }

        partition_ptr part =res.mountpoint;
        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        quasi_stat_t hio_stat;
        quasi_stat_t vio_stat;

        if (part->IsHostMounted())
        {
            fs::path host_path_target{};
            if (int hostpath_status = part->GetHostPath(host_path_target, res.local_path); hostpath_status != 0)
                return hostpath_status;

            if (hio_status = this->hio_driver.Stat(host_path_target, &hio_stat); 0 != hio_status)
                // hosts operation must succeed in order to continue
                return hio_status;

            host_used = true;
        }

        this->vio_driver.SetCtx(&res,host_used, nullptr);
        vio_status = this->vio_driver.Stat(res.local_path, &vio_stat);
        this->vio_driver.ClearCtx();

        if (host_used)
        {
            vio_stat.st_mode = hio_stat.st_mode;
            vio_stat.st_size = hio_stat.st_size;
            vio_stat.st_blksize = hio_stat.st_blksize;
            vio_stat.st_blocks = hio_stat.st_blocks;
            vio_stat.st_atim = hio_stat.st_atim;
            vio_stat.st_mtim = hio_stat.st_mtim;
            vio_stat.st_ctim = hio_stat.st_ctim;
        }

        memcpy(statbuf, &vio_stat, sizeof(quasi_stat_t));

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }

    int QFS::FStat(const int fd, quasi_stat_t *statbuf)
    {
        fd_handle_ptr handle = GetHandle(fd);
        if (nullptr == handle)
            return -QUASI_EBADF;

        bool host_used = false;
        int hio_status = 0;
        int vio_status = 0;

        quasi_stat_t hio_stat;
        quasi_stat_t vio_stat;

        if (handle->IsHostBound())
        {
            int host_fd = handle->host_fd;
            hio_status = this->hio_driver.FStat(host_fd, &hio_stat);
            host_used = true;
        }

        this->vio_driver.SetCtx(nullptr, host_used, handle);
        vio_status = this->vio_driver.FStat(fd, &vio_stat);
        this->vio_driver.ClearCtx();

        if (host_used)
        {
            vio_stat.st_mode = hio_stat.st_mode;
            vio_stat.st_size = hio_stat.st_size;
            vio_stat.st_blksize = hio_stat.st_blksize;
            vio_stat.st_blocks = hio_stat.st_blocks;
            vio_stat.st_atim = hio_stat.st_atim;
            vio_stat.st_mtim = hio_stat.st_mtim;
            vio_stat.st_ctim = hio_stat.st_ctim;
        }

        memcpy(statbuf, &vio_stat, sizeof(quasi_stat_t));

        if (host_used && (hio_status != vio_status))
            LogError("Host returned {}, but virtual driver returned {}", hio_status, vio_status);

        return vio_status;
    }
}
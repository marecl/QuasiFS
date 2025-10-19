// INAA License @marecl 2025

#include "../quasi_errno.h"
#include "../quasi_types.h"

#include "../quasifs_partition.h"
#include "../quasifs_inode_directory.h"
#include "../quasifs_inode_regularfile.h"
#include "../quasifs_inode_symlink.h"

#include "../../log.h"

namespace QuasiFS
{
    Partition::Partition(const fs::path &host_root, const int root_permissions) : block_id(next_block_id++), host_root(host_root.lexically_normal())
    {
        this->root = Directory::Create();
        // clear defaults, write
        chmod(this->root, root_permissions);
        IndexInode(this->root);
        mkrelative(this->root, this->root);
    };

    fs::path Partition::SanitizePath(const fs::path &path)
    {
        // lexically normal to resolve relative calls
        // avoids going OOB with malicious file names
        fs::path tmp = path.lexically_normal();
        if (tmp.string().find(this->host_root, 0) == 0)
            return tmp;

        return {};
    }

    int Partition::GetHostPath(fs::path &output_path, const fs::path &local_path)
    {
        if (this->host_root.empty())
            return -QUASI_ENODEV;

        // must be relative to root, otherwise lvalue is overwritten
        fs::path host_path_target = (this->host_root / local_path.lexically_relative("/"));
        if (host_path_target.empty())
            return -QUASI_EINVAL;

        fs::path host_path_target_sanitized = SanitizePath(host_path_target);
        if (host_path_target_sanitized.empty())
        {
            LogError("Malicious path detected: {}", host_path_target.string());
            return -QUASI_EACCES;
        }
        output_path = host_path_target_sanitized;
        // Log("Resolving local {} to {}", local_path.string(), host_path_target_sanitized.string());
        return 0;
    }

    inode_ptr Partition::GetInodeByFileno(fileno_t fileno)
    {
        auto ret = inode_table.find(fileno);
        return (inode_table.end() == ret) ? nullptr : ret->second;
    }

    int Partition::Resolve(fs::path &path, Resolved &res)
    {
        if (path.empty())
            return -QUASI_EINVAL;

        if (path.is_relative())
            return -QUASI_EBADF;

        res.mountpoint = shared_from_this();
        res.local_path = "/";
        res.parent = this->root;
        res.node = this->root;
        res.leaf = "";

        // these hold up between iterations, but setting them can be ignored if the function is about to return
        dir_ptr parent = res.parent;
        inode_ptr current = res.node;

        bool is_final = false;

        for (auto part = path.begin(); part != path.end(); part++)
        {
            is_final = std::next(part) == path.end();

            if ("/" == *part)
            {
                res.local_path = "/";
                res.parent = res.parent;
                res.node = res.node;
                res.leaf = "/";
                continue;
            }

            // empty means trailing / which is interpreted only when using dirs or symlinks
            if (part->empty())
            {
                if (!is_final)
                    // something went wrong
                    throw 666;

                if (!(current->is_link() || current->is_dir()))
                    // trailing slash after a file
                    return -QUASI_ENOTDIR;

                return 0;
            }

            if (!(current->is_link() || current->is_dir()) && !is_final)
            {
                // path elements must be a dir or a symlink
                return -QUASI_ENOTDIR;
            }

            //

            if (current->is_dir())
            {
                if (!current->CanRead())
                    return -QUASI_EACCES;

                dir_ptr dir = std::static_pointer_cast<Directory>(current);
                parent = dir;
                current = dir->lookup(*part);

                res.local_path /= *part;
                res.parent = parent;
                res.node = current;
                res.leaf = *part;
            }

            // file not found in current directory, ENOENT
            if (nullptr == res.node)
            {
                // zero out everything
                // avoids a condition where calling function may think that parent
                // directory is real parent directory, when in fact we might just stop in the
                // middle of the path
                if (!is_final)
                {
                    res.node = nullptr;
                    res.parent = nullptr;
                }
                return -QUASI_ENOENT;
            }

            //

            /**
             * WARNING
             * These relations must be saved as-is
             * Currently CWD is NOT implemented. Might be, might not in the future.
             * For current purposes all irrelevant Resolved members could be set to nullptr,
             * but then i'll forget what they were supposed to be.
             * They need to be saved to build CWD history and enable relative paths (since we may jump over parents)
             */

            // quick lookahead if this directory is a mountpoint
            if (current->is_dir())
            {
                dir_ptr current_dir = std::static_pointer_cast<Directory>(current);

                if (current_dir->mounted_root != nullptr)
                {
                    // if it's a mountpoint, the preceeding path is invalid from target partition's POV
                    // we take remainder of the path (current leaf name belongs to upstream) and leave everything
                    // AFTER host's path
                    // since QFS holds mountpoint meta, its it's job to resolve mounted root directory.
                    fs::path remainder = "/";
                    for (auto p = std::next(part); p != path.end(); p++)
                        remainder /= *p;
                    path = remainder;

                    res.parent = current_dir; // no point, unused in this context
                    res.node = current_dir->mounted_root;
                    res.leaf = *part;

                    return 0;
                }
            }

            //

            if (current->is_link())
            {
                // just like with mountpoints, we discard everything up until the "next" element in path
                fs::path remainder = "";
                for (auto p = std::next(part); p != path.end(); p++)
                    remainder /= *p;
                path = remainder;

                res.parent = parent;
                res.node = current;
                res.leaf = *part;
                return 0;
            }
        }

        return 0;
    }

    int Partition::touch(dir_ptr parent, const std::string &name, inode_ptr child)
    {
        if (nullptr == parent)
            return -QUASI_EINVAL;

        auto ret = parent->link(name, child);
        if (ret == 0)
            IndexInode(child);
        return ret;
    }

    int Partition::mkdir(dir_ptr parent, const std::string &name)
    {
        return mkdir(parent, name, Directory::Create());
    }

    int Partition::mkdir(dir_ptr parent, const std::string &name, dir_ptr child)
    {
        if (nullptr == parent)
            return -QUASI_ENOENT;

        int ret = parent->link(name, child);

        if (ret == 0)
            IndexInode(child);

        auto real_parent = parent->mounted_root ? parent->mounted_root : parent;
        mkrelative(real_parent, child);

        return ret;
    }

    int Partition::rmdir(fs::path path)
    {
        Resolved res;
        int resolve_status = this->Resolve(path, res);

        if (resolve_status != 0)
            return resolve_status;

        return this->rmdir(res.parent, res.leaf);
    }

    int Partition::rmdir(dir_ptr parent, const std::string &name)
    {
        if (nullptr == parent)
            return -QUASI_ENOENT;
        if (name.empty())
            return -QUASI_EINVAL;

        inode_ptr target = parent->lookup(name);
        if (nullptr == target)
            return -QUASI_ENOENT;

        if (!target->is_dir())
            return -QUASI_ENOTDIR;

        int status = parent->unlink(name);
        if (0 != status)
            return status;

        return rmInode(target);
    }

    int Partition::link(inode_ptr source, dir_ptr destination_parent, const std::string &name)
    {
        if (nullptr == source || nullptr == destination_parent)
            return -QUASI_ENOENT;
        if (name.empty())
            return -QUASI_EINVAL;
        if (destination_parent->lookup(name) != nullptr)
            return -QUASI_EEXIST;
        if (source->is_dir())
            return -QUASI_EPERM;

        return destination_parent->link(name, source);
    }

    int Partition::unlink(dir_ptr parent, const std::string &name)
    {
        if (nullptr == parent)
            return -QUASI_ENOENT;
        if (name.empty())
            return -QUASI_EINVAL;

        inode_ptr target = parent->lookup(name);
        if (nullptr == target)
            return -QUASI_ENOENT;

        if (target->is_dir())
            return -QUASI_EISDIR;

        int status = parent->unlink(name);
        if (0 != status)
            return status;

        return rmInode(target);
    }

    int Partition::chmod(inode_ptr target, mode_t mode)
    {
        if (nullptr == target)
            return -QUASI_EINVAL;

        return target->chmod(mode);
    }

    int Partition::rmInode(fileno_t fileno)
    {
        inode_ptr target = GetInodeByFileno(fileno);
        if (nullptr == target)
            return -QUASI_EINVAL;

        return rmInode(target);
    }

    int Partition::rmInode(inode_ptr node)
    {
        if (nullptr == node)
            return -QUASI_EINVAL;

        // truly remove if there are no hardlinks
        if (node->st.st_nlink > 0)
            return 0;

        // TODO: check for open file handles, return -QUASI_EEBUSY

        this->inode_table.erase(node->GetFileno());
        return 0;
    }

    bool Partition::IndexInode(inode_ptr node)
    {
        if (nullptr == node)
            return false;

        // Assign fileno and add it to the fs table
        fileno_t node_fileno = node->GetFileno();
        if (node_fileno == -1)
            node_fileno = node->SetFileno(this->NextFileno());

        inode_table[node_fileno] = node;
        if (node->is_dir())
        {
            auto dir = std::static_pointer_cast<Directory>(node);
            for (auto &kv : dir->entries)
                IndexInode(kv.second);
            if (dir->mounted_root)
                IndexInode(dir->mounted_root);
        }

        node->st.st_ino = node_fileno;
        node->st.st_dev = block_id;

        return true;
    }

    void Partition::mkrelative(dir_ptr parent, dir_ptr child)
    {
        child->link(".", child);
        child->link("..", parent);
    }
};
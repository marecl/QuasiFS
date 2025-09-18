#include "include/quasifs_types.h"
#include "include/quasifs_partition.h"
#include "include/quasifs_inode_directory.h"
#include "include/quasifs_inode_regularfile.h"
#include "include/quasifs_inode_symlink.h"

namespace QuasiFS
{

    Partition::Partition() : block_id(next_block_id++)
    {
        this->root = Inode::Create<Directory>();
        IndexInode(this->root);
        mkrelative(this->root, this->root);
    };

    static partition_ptr Create(void)
    {
        partition_ptr part = std::make_shared<Partition>();
        return part;
    }

    int Partition::resolve(fs::path &path, Resolved &r)
    {
        if (path.empty())
            return -ENOENT;

        r.parent = this->root;
        r.node = this->root;
        r.leaf = "";

        // these hold up between iterations, but setting them can be ignored if the function is about to returnI
        dir_ptr parent = r.parent;
        inode_ptr current = r.node;

        bool is_final = false;

        std::string pathstr = path.string();
        for (auto part = path.begin(); part != path.end(); part++)
        {

            std::string partstr = part->string();
            if (part->empty() || *part == "/")
                continue;

            is_final = std::next(part) == path.end();

            // non-final element can't be anything other than a dir or link
            if (!(current->is_link() || current->is_dir()) && !is_final)
                return -ENOTDIR;

            // link - return with mountpoint and remaining path
            // to be resolved by superblock, just check if `node` is a link,
            // resolve (based in root) symlink path + remainder
            if (current->is_link())
            {
                // Symlink can point to a directory
                fs::path remainder = "";
                for (auto p = std::next(part); p != path.end(); p++)
                    remainder /= *p;
                path = remainder;

                r.parent = parent;
                r.node = current;
                r.leaf = partstr;
                return 0;
            }

            if (current->is_dir())
            {
                dir_ptr dir = std::static_pointer_cast<Directory>(current);
                parent = dir;
                current = dir->lookup(partstr);

                r.parent = parent;
                r.node = current;
                r.leaf = partstr;
            }

            // file not found in current directory, ENOENT
            if (nullptr == r.node)
                return -ENOENT;

            // quick lookahead if this directory is a mountpoint
            if (current->is_dir())
            {
                dir_ptr dir = std::static_pointer_cast<Directory>(current);
                // same as symlink, if mountpoint is encountered, pass it on to superblock
                if (dir->mounted_root != nullptr)
                {
                    fs::path remainder = "";
                    for (auto p = std::next(part); p != path.end(); p++)
                        remainder /= *p;
                    path = remainder;

                    r.parent = dir; // no point, unused in this context
                    r.node = dir->mounted_root;
                    r.leaf = partstr;

                    return 0;
                }
            }
        }

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

    int Partition::rmInode(fileno_t fileno)
    {
        inode_ptr target = GetInodeByFileno(fileno);
        if (nullptr == target)
            return -ENOENT;
        return rmInode(target);
    }

    int Partition::rmInode(inode_ptr node)
    {
        if (nullptr == node)
            return -ENOENT;

        if (node->st.st_nlink > 0)
            return 0;
        // TODO: check for open file handles, return -EEBUSY

        this->inode_table.erase(node->GetFileno());
        return 0;
    }

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    int Partition::touch(dir_ptr node, const std::string &name)
    {
        return this->touch(node, name, Inode::Create<RegularFile>());
    }

    int Partition::touch(dir_ptr parent, const std::string &name, file_ptr child)
    {
        if (nullptr == parent)
            return -1;

        auto ret = parent->link(name, child);
        if (ret == 0)
            IndexInode(child);
        return ret;
    }

    int Partition::rm(fileno_t fileno)
    {
        if (fileno == -1)
            return -EINVAL;

        inode_ptr target = GetInodeByFileno(fileno);
        if (nullptr == target)
            return -ENOENT;

        return rm(target);
    }

    int Partition::rm(fs::path path)
    {
        if (path.empty())
            return -EINVAL;

        // resolve name
        // Resolved r =
        inode_ptr i = nullptr;
        return rm(i);
    }

    int Partition::rm(inode_ptr node)
    {
        if (nullptr == node)
            return -ENOENT;
        return 0;
    }

    int Partition::mkdir(dir_ptr parent, const std::string &name)
    {
        return mkdir(parent, name, Inode::Create<Directory>());
    }

    int Partition::mkdir(dir_ptr parent, const std::string &name, dir_ptr child)
    {
        if (nullptr == parent)
            return -1;

        int ret = parent->mkdir(name, child);

        if (ret == 0)
            IndexInode(child);

        auto real_parent = parent->mounted_root ? parent->mounted_root : parent;
        mkrelative(real_parent, child);

        return ret;
    }

    int Partition::rmdir(fs::path path)
    {
        return -EINVAL;
    }
    int Partition::rmdir(dir_ptr parent, const std::string &name)
    {
        return -EINVAL;
    }

    void Partition::mkrelative(dir_ptr parent, dir_ptr child)
    {
        child->mkdir(".", child);
        child->mkdir("..", parent);
    }

    int Partition::unlink(dir_ptr parent, const std::string &child)
    {
        inode_ptr target = parent->lookup(child);
        if (nullptr == target)
            return -ENOENT;

        int status = parent->unlink(child);
        if (0 != status)
            return status;

        return rmInode(target);
    }

    inode_ptr Partition::GetInodeByFileno(fileno_t fileno)
    {
        auto ret = inode_table.find(fileno);
        return (ret == inode_table.end()) ? nullptr : ret->second;
    }

};
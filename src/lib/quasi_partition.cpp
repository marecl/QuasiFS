#include "include/quasi_types.h"
#include "include/quasi_partition.h"
#include "include/quasi_inode_directory.h"
#include "include/quasi_inode_regularfile.h"
#include "include/quasi_inode_symlink.h"

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

    inode_ptr Partition::GetInode(fileno_t fileno)
    {
        auto ret = inode_table.find(fileno);
        return (ret == inode_table.end()) ? nullptr : ret->second;
    }

    Resolved Partition::resolve(fs::path path)
    {
        if (path.empty() || path.is_relative())
            return {};

        Resolved r{};
        r.mountpoint = shared_from_this();

        if (path == "/")
        {
            r.parent = this->root;
            r.node = this->root;
            r.leaf = "/";
            return r;
        }

        r.node = nullptr;
        r.leaf = "";

        dir_ptr parent = this->root;
        inode_ptr current = this->root;

        for (auto part = path.begin(); part != path.end(); part++)
        {
            std::string partstr = part->string();

            // current node doesn't exist
            if (nullptr == current)
            {
                r.parent = parent;
                r.node = nullptr;
                r.leaf = "";
            }

            // file encountered at any point in time
            if (current->is_file())
            {
                r.parent = parent;
                r.node = current;
                r.leaf = partstr;
                return r;
            }

            if (current->is_dir())
            {
                dir_ptr dir = std::dynamic_pointer_cast<Directory>(current);
                if (dir->mounted_root != nullptr)
                {
                    fileno_t mountpoint_fileno = dir->mounted_root->GetFileno();
                    auto target_partition = this->fs_table.find(mountpoint_fileno);
                    if (target_partition == this->fs_table.end())
                    {
                        // linked partition not found
                        r.parent = parent;
                        r.node = nullptr;
                        r.leaf = "";
                        return r;
                    }

                    partition_ptr chain_part = target_partition->second;
                    fs::path remainder;
                    for (auto rem_part = std::next(part); rem_part != path.end(); ++rem_part)
                        remainder /= *rem_part;
                    return chain_part->resolve(remainder);
                }

                parent = dir;
                current = parent->lookup(partstr);
                continue;
            }
        }

        return r;
    }

    bool Partition::IndexInode(inode_ptr node)
    {
        if (nullptr == node)
            return false;

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
        node->st.ino = node_fileno;
        node->st.dev = block_id;

        return true;
    }

    bool Partition::RemoveInode(inode_ptr node)
    {
        if (!node)
            return false;
        fileno_t node_fileno = node->GetFileno();
        if (node_fileno == -1)
        {
            // some other error, unindexed node
            return false;
        }

        if (node->st.nlink > 1)
            return false; // error, file in use

        inode_table.erase(node_fileno);

        return true;
    }

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    int Partition::touch(dir_ptr node, std::string leaf)
    {
        return this->touch(node, Inode::Create<RegularFile>(), leaf);
    }

    int Partition::touch(dir_ptr node, file_ptr new_node, std::string leaf)
    {
        if (node == nullptr)
            return -1;

        auto dir = std::static_pointer_cast<Directory>(node);
        auto ret = dir->link(new_node, leaf);
        if (ret == 0)
            IndexInode(new_node);
        return ret;
    }

    int Partition::rm(fileno_t fileno)
    {
        if (fileno == -1)
            return -EINVAL;

        auto target = this->inode_table.find(fileno);
        if (target == this->inode_table.end())
            return -ENOENT;

        return rm(target->second);
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

    int Partition::mkdir(dir_ptr node, std::string leaf)
    {
        return mkdir(node, Inode::Create<Directory>(), leaf);
    }

    int Partition::mkdir(dir_ptr node, dir_ptr new_node, std::string leaf)
    {
        if (node == nullptr)
            return -1;

        auto dir = std::dynamic_pointer_cast<Directory>(node);
        int ret = dir->mkdir(leaf, new_node);

        if (ret == 0)
            IndexInode(new_node);

        auto real_parent = node->mounted_root ? node->mounted_root : node;
        mkrelative(real_parent, new_node);

        return ret;
    }

    int Partition::rmdir(fs::path path)
    {
        return 0;
    }
    int Partition::rmdir(inode_ptr parent, std::string leaf)
    {
        return 0;
    }

    int Partition::rmInode(fileno_t target)
    {
        auto t = this->inode_table.find(target);
        if (t == this->inode_table.end())
            return -ENOENT;
        return rmInode(t->second);
    }

    int Partition::rmInode(inode_ptr target)
    {
        if (target == nullptr)
            return -ENOENT;

        auto nlink = &target->st.nlink;
        *nlink = *nlink == 1 ? 0 : (*nlink)--;

        if (*nlink != 0)
            return -EBUSY;

        this->inode_table.erase(target->GetFileno());
        return 0;
    }

    void Partition::mkrelative(dir_ptr parent, dir_ptr child)
    {
        child->link(child, ".");
        child->link(parent, "..");
    }

};
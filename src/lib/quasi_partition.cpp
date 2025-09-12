#include "include/quasi_types.h"
#include "include/quasi_partition.h"
#include "include/quasi_inode_directory.h"
#include "include/quasi_inode_regularfile.h"

namespace QuasiFS
{

    Partition::Partition() : block_id(next_block_id++)
    {
        this->root = std::make_shared<Directory>();
        IndexInode(this->root);
    };

    static partition_ptr Create(void)
    {
        return std::make_shared<Partition>();
    }

    inode_ptr Partition::GetInode(fileno_t fileno)
    {
        auto ret = inode_table.find(fileno);
        return (ret == inode_table.end()) ? nullptr : ret->second;
    }

    void Partition::IndexInode(inode_ptr node)
    {
        if (!node)
            return;
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
        node->st.nlink = 1;
        node->st.dev = block_id;
    }

    // create file at path (creates entry in parent dir). returns 0 or negative errno
    int Partition::touch(dir_ptr node, std::string leaf)
    {
        return this->touch(node, std::make_shared<RegularFile>(), leaf);
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

    int Partition::mkdir(dir_ptr node, std::string leaf)
    {
        return mkdir(node, std::make_shared<Directory>(), leaf);
    }

    int Partition::mkdir(dir_ptr node, dir_ptr new_node, std::string leaf)
    {
        if (node == nullptr)
            return -1;

        // TODO: create . and .. here
        auto dir = std::static_pointer_cast<Directory>(node);
        int ret = dir->mkdir(leaf, new_node);
        if (ret == 0)
            IndexInode(new_node);
        return ret;
    }

};
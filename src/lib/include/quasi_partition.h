#pragma once

#include <unordered_map>

#include "quasi_types.h"

namespace QuasiFS
{

    class Partition : std::enable_shared_from_this<Partition>
    {
    private:
        fileno_t NextFileno(void) { return this->next_fileno++; };

        // file list
        std::unordered_map<fileno_t, inode_ptr> inode_table{};

        dir_ptr root;
        fileno_t next_fileno = 2;
        const blkid_t block_id;

        static inline blkid_t next_block_id = 1;

    public:
        // for nested mountpoints
        std::unordered_map<fileno_t, partition_ptr> fs_table{};

        Partition();
        ~Partition() = default;

        static partition_ptr Create(void)
        {
            return std::make_shared<Partition>();
        }

        dir_ptr GetRoot(void) { return this->root; };
        inode_ptr GetInode(fileno_t fileno);
        blkid_t GetBlkId(void) { return this->block_id; };

        Resolved resolve(fs::path path);

        bool IndexInode(inode_ptr node);
        bool RemoveInode(inode_ptr node);

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int touch(dir_ptr node, std::string leaf);
        int touch(dir_ptr node, file_ptr new_node, std::string leaf);

        int rm(fileno_t fileno);
        int rm(fs::path path);
        int rm(inode_ptr node);

        int mkdir(dir_ptr node, std::string leaf);
        int mkdir(dir_ptr node, dir_ptr new_node, std::string leaf);

        int rmdir(fs::path path);
        int rmdir(inode_ptr parent, std::string leaf);

        int rmInode(fileno_t target);
        int rmInode(inode_ptr target);

        void mkrelative(dir_ptr parent, dir_ptr child);
    };

};
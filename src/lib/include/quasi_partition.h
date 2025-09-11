#pragma once

#include <unordered_map>

#include "quasi_inode.h"
#include "quasi_inode_directory.h"
#include "quasi_inode_regularfile.h"

namespace QuasiFS
{

    class Partition
    {
    public:
        Partition();
        ~Partition() = default;

        static partition_ptr Create(void)
        {
            return std::make_shared<Partition>();
        }

        dir_ptr GetRoot(void) { return this->root; };
        inode_ptr GetInode(fileno_t fileno);
        blkid_t GetBlkid(void) { return this->block_id; };

        void IndexInode(inode_ptr node);

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int touch(dir_ptr node, std::string leaf);
        int touch(dir_ptr node, file_ptr new_node, std::string leaf);

        int mkdir(dir_ptr node, std::string leaf);
        int mkdir(dir_ptr node, dir_ptr new_node, std::string leaf);

    private:
        fileno_t NextFileno(void) { return this->next_fileno++; };
        // wlasne inode
        std::unordered_map<fileno_t, inode_ptr> inode_table;

        dir_ptr root;
        fileno_t next_fileno = 2;
        const blkid_t block_id;

        static inline blkid_t next_block_id = 1;
    };

};
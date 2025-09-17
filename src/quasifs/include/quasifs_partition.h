#pragma once

#include <unordered_map>

#include "quasifs_types.h"

namespace QuasiFS
{

    class Partition : public std::enable_shared_from_this<Partition>
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
        Partition();
        ~Partition() = default;

        static partition_ptr Create(void)
        {
            return std::make_shared<Partition>();
        }

        dir_ptr GetRoot(void) { return this->root; };
        inode_ptr GetInode(fileno_t fileno);
        blkid_t GetBlkId(void) { return this->block_id; };

        int resolve(fs::path &path, Resolved &r);

        bool IndexInode(inode_ptr node);
        int rmInode(fileno_t target);
        int rmInode(inode_ptr target);

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int touch(dir_ptr parent, const std::string &name);
        int touch(dir_ptr parent, const std::string &name, file_ptr child);

        int rm(fileno_t fileno);
        int rm(fs::path path);
        int rm(inode_ptr node);

        int mkdir(dir_ptr parent, const std::string &name);
        int mkdir(dir_ptr parent, const std::string &name, dir_ptr child);

        int rmdir(fs::path path);
        int rmdir(dir_ptr parent, const std::string &name);

        static void mkrelative(dir_ptr parent, dir_ptr child);

        int unlink(dir_ptr parent, const std::string& child);
    };

};
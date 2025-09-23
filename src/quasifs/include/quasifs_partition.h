#pragma once

#include <unordered_map>

#include "quasifs_types.h"
#include "../../log.h"

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

        const fs::path host_root{};

    public:
        Partition(const fs::path &host_root = "");
        ~Partition() = default;

        template <typename... Args>
        static partition_ptr Create(Args &&...args)
        {
            return std::make_shared<Partition>(std::forward<Args>(args)...);
        }

        fs::path SanitizePath(const fs::path &path)
        {
            // lexically normal to resolve relative calls
            // avoids going OOB with malicious file names
            fs::path tmp = path.lexically_normal();
            if (tmp.string().find(this->host_root, 0) == 0)
                return tmp;

            return {};
        }

        // return - valid, out_path - sanitized path
        bool GetHostPath(fs::path &output_path, const fs::path &local_path = "/")
        {
            // must be relative to root, otherwise lvalue is overwritten
            fs::path host_path_target = (this->GetHostRoot() / local_path.lexically_relative("/"));
            fs::path host_path_target_sanitized = SanitizePath(host_path_target);
            if (host_path_target_sanitized.empty())
            {
                LogError("Malicious path detected: {}", host_path_target.string());
                return false;
            }
            output_path = host_path_target_sanitized;
            Log("Resolving local {} to {}", local_path.string(), host_path_target_sanitized.string());
            return true;
        }

        dir_ptr GetRoot(void) { return this->root; }
        bool IsHostMounted(void) { return !this->host_root.empty(); }
        const fs::path GetHostRoot(void) { return this->host_root; }
        blkid_t GetBlkId(void) { return this->block_id; }
        inode_ptr GetInodeByFileno(fileno_t fileno);

        int Resolve(fs::path &path, Resolved &r);

        bool IndexInode(inode_ptr node);
        int rmInode(fileno_t fileno);
        int rmInode(inode_ptr node);

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

        int unlink(dir_ptr parent, const std::string &child);
    };

};
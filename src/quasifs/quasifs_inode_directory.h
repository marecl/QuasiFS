// INAA License @marecl 2025

#pragma once

#include <map>
#include <string>

#include "quasi_sys_stat.h"
#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS
{

    // Directory
    class Directory : public Inode
    {
    public:
        std::map<std::string, inode_ptr> entries{};
        dir_ptr mounted_root = nullptr;

        Directory();
        ~Directory() = default;

        static dir_ptr Create(void)
        {
            return std::make_shared<Directory>();
        }

        //
        // Inode overrides
        //

        virtual quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override { return -QUASI_EISDIR; }
        virtual quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override { return -QUASI_EISDIR; }
        virtual int fstat(quasi_stat_t *stat) override
        {
            this->st.st_size = entries.size() * 32;
            *stat = st;
            return 0;
        }

        //
        // Dir-specific
        //

        // Find an element with [name]
        inode_ptr lookup(const std::string &name);

        // Add hardlink to [child] with [name]
        int link(const std::string &name, inode_ptr child);
        // Remove hardlink to [name]
        int unlink(const std::string &name);
        // list entries
        std::vector<std::string> list();
    };

}
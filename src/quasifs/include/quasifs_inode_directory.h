#pragma once

#include <map>
#include <string>

#include "quasifs_types.h"
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

        // Insert [child] directory with [name]
        int mkdir(const std::string &name, dir_ptr child);

        // Find an element with [name]
        inode_ptr lookup(const std::string &name);

        // Add hardlink to [child] with [name]
        int link(const std::string &name, inode_ptr child);
        // Remove hardlink to [name]
        int unlink(const std::string &name);
        // list entries
        std::vector<std::string> list();
        quasi_stat_t getattr();
    };

}
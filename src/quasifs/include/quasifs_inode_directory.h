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

        int mkdir(const std::string &name, dir_ptr child);
        // add rmdir

        inode_ptr lookup(const std::string &name);

        int link(const std::string &name, inode_ptr child);
        int unlink(const std::string &name);

        std::vector<std::string> list();
        Stat getattr();
    };

}
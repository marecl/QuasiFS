#pragma once

#include <map>
#include <string>

#include "quasi_types.h"
#include "quasi_inode.h"

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

        bool is_dir() const override { return true; }

        int mkdir(const std::string &name, dir_ptr inode) override;
        // add rmdir

        inode_ptr lookup(const std::string &name) override;

        int link(inode_ptr inode, const std::string &name) override;
        int unlink(const std::string &name) override;

        std::vector<std::string> list() override;
        Stat getattr() override;
    };

}
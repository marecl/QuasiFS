#pragma once

#include "quasifs_types.h"
#include "quasifs_inode.h"

namespace QuasiFS
{

    class Symlink : public Inode
    {
        const fs::path target;

    public:
        Symlink(fs::path target);
        ~Symlink() = default;

        static symlink_ptr Create(fs::path target)
        {
            return std::make_shared<Symlink>(target);
        }

        fs::path follow(void);
        quasi_stat_t getattr(inode_ptr inode);
    };

}
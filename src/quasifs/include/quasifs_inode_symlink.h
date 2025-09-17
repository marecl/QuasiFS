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

        fs::path follow(void);
        Stat getattr(inode_ptr inode);
    };

}
#pragma once

#include "quasi_types.h"
#include "quasi_inode.h"

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
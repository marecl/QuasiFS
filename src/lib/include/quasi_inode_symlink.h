#pragma once

#include "quasi_types.h"
#include "quasi_inode.h"

namespace QuasiFS
{

    class Symlink : public Inode
    {
        const fs::path target;

    public:
        Symlink(fs::path target) : target(target)
        {
            // fileno and blkdev assigned by partition
            this->st.nlink = 1;
            this->st.size = target.string().size();
            this->st.mode = 0000644 | S_IFLNK;
            // not incrementing target, this type is a softlink
        }
        ~Symlink() = default;

        fs::path follow(void)
        {
            return target;
        }

        // if inode is invalid, return symlink stat
        Stat getattr(inode_ptr inode)
        {
            if (nullptr == inode)
                return this->st;
            return inode->st;
        }
    };

}
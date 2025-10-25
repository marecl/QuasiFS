// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS
{

    class Device : public Inode
    {

    public:
        Device()
        {
            // fileno and blkdev assigned by partition
            this->st.st_size = 0;
            this->st.st_blksize = 0;
            this->st.st_blocks = 0;

            this->st.st_mode = 0000755 | QUASI_S_IFCHR;
            this->st.st_nlink = 0;
            // not incrementing target, this type is a softlink
        }
        ~Device() = default;
    };

}
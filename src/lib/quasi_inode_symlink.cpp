#include "include/quasi_inode_symlink.h"

namespace QuasiFS
{

    Symlink::Symlink(fs::path target) : target(target)
    {
        // fileno and blkdev assigned by partition
        this->st.st_size = target.string().size();
        this->st.st_mode = 0000644 | S_IFLNK;
        this->st.st_nlink = 0;
        // not incrementing target, this type is a softlink
    }

    fs::path Symlink::follow(void)
    {
        return target;
    }

    // if inode is invalid, return symlink stat
    Stat Symlink::getattr(inode_ptr inode)
    {
        if (nullptr == inode)
            return this->st;
        return inode->st;
    }
}
#include <cstring>

#include "include/quasi_inode_regularfile.h"

namespace QuasiFS
{

    RegularFile::RegularFile()
    {
        st.mode = 0000644 | S_IFREG;
        st.nlink = 0;
    }

    ssize_t RegularFile::read(off_t offset, void *buf, size_t count)
    {
        memcpy(buf, "XD\0", 3);
        return 3;
    }

    ssize_t RegularFile::write(off_t offset, const void *buf, size_t count)
    {
        return count;
    }

}
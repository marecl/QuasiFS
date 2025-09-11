#include <cstring>
#include "include/quasi_types.h"
#include "include/quasi_inode_regularfile.h"

namespace QuasiFS
{

    RegularFile::RegularFile() { st.mode = 0100644; /* regular file */ }
    bool RegularFile::is_file() const  { return true; }

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
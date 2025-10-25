// INAA License @marecl 2025

#include <vector>

#include "../quasi_errno.h"
#include "../quasifs_inode_regularfile.h"

namespace QuasiFS
{

    quasi_ssize_t RegularFile::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        auto size = &this->st.st_size;
        auto end_pos = offset + count;

        return end_pos > *size ? *size - offset : count;
    }

    quasi_ssize_t RegularFile::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        auto size = &this->st.st_size;
        auto end_pos = offset + count;

        *size = end_pos > *size ? end_pos : *size;

        return count;
    }

int RegularFile::ftruncate(quasi_off_t length)
    {
        if (length < 0)
            return -QUASI_EINVAL;
        this->st.st_size = length;
        return 0;
    }

} // namespace QuasiFS
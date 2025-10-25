// INAA License @marecl 2025

#include <vector>

#include "../quasi_errno.h"
#include "../quasifs_inode_virtualfile.h"

namespace QuasiFS
{

    quasi_ssize_t VirtualFile::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        int64_t idx;
        int64_t read_amt = this->data.size() - offset - count;

        // if >= 0 - we're good to go
        // <0 - n-bytes are missing, won't enter loop
        read_amt = count + read_amt * (read_amt < 0);

        for (idx = 0; idx < read_amt; idx++)
        {
            char c = this->data.at(idx + offset);
            static_cast<char *>(buf)[idx] = c;
        }

        return read_amt;
    }

    quasi_ssize_t VirtualFile::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        auto size = &this->st.st_size;
        auto end_pos = offset + count;
        *size = end_pos > *size ? end_pos : *size;

        // size can only be greater, so it will always scale up
        this->data.resize(*size, 0);

        for (uint64_t idx = offset; idx < *size; idx++)
            this->data[idx] = static_cast<const char *>(buf)[idx];

        return count;
    }

    int VirtualFile::ftruncate(quasi_off_t length)
    {
        if (length < 0)
            return -QUASI_EINVAL;
        this->data.resize(length, 0);
        this->st.st_size = length;
        return 0;
    }

} // namespace QuasiFS
// INAA License @marecl 2025

#include <vector>

#include "../quasifs_inode_regularfile.h"

namespace QuasiFS
{

    RegularFile::RegularFile()
    {
        st.st_mode = 0000755 | QUASI_S_IFREG;
        st.st_nlink = 0;
    }

    quasi_ssize_t RegularFile::read(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        quasi_ssize_t idx;
        quasi_off_t read_amt = this->data.size() - offset - count;

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

    quasi_ssize_t RegularFile::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        auto size = &this->st.st_size;
        auto end_pos = offset + count;
        *size = end_pos > *size ? end_pos : *size;

        // size can only be greater, so it will always scale up
        this->data.resize(*size, 0);

        for (quasi_off_t idx = offset; idx < *size; idx++)
            this->data[idx] = static_cast<const char *>(buf)[idx];

        return count;
    }

    int RegularFile::ftruncate(quasi_off_t length)
    {
        if (length < 0)
            return -QUASI_EINVAL;
        this->data.resize(length, 0);
        this->st.st_size = length;
        return 0;
    }

    quasi_ssize_t RegularFile::MockRead(quasi_off_t offset, void *buf, quasi_size_t count)
    {
        auto size = &this->st.st_size;
        auto end_pos = offset + count;

        return end_pos > *size ? *size - offset : count;
    }

    quasi_ssize_t RegularFile::MockWrite(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        auto size = &this->st.st_size;
        auto end_pos = offset + count;

        *size = end_pos > *size ? end_pos : *size;

        return count;
    }

    int RegularFile::MockTruncate(quasi_off_t length)
    {
        if (length < 0)
            return -QUASI_EINVAL;
        this->st.st_size = length;
        return 0;
    }
}
#include <vector>

#include "include/quasifs_inode_regularfile.h"

namespace QuasiFS
{

    RegularFile::RegularFile()
    {
        st.st_mode = 0000755 | S_IFREG;
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
        quasi_off_t idx = 0;

        // write overlapping data
        for (idx = 0; idx + offset < this->data.size(); idx++)
        {
            this->data[idx + offset] = static_cast<const char *>(buf)[idx];
        }

        // append
        quasi_size_t maxidx = this->data.size() - offset + count;
        for (; idx < maxidx; idx++)
        {
            char c = static_cast<const char *>(buf)[idx];
            this->data.push_back(c);
        }

        this->st.st_size += idx;

        return count;
    }

    int RegularFile::truncate(quasi_off_t length)
    {
        if (length < 0)
            return -QUASI_EINVAL;
        this->data.resize(length, 0);
        this->st.st_size = length;
        return length;
    }
}
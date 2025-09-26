#include <cstring>
#include <iostream>

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
        quasi_size_t idx = 0;
        quasi_size_t max = this->data.size();
        max = count < max ? count : max;

        std::cout << "Reading from file: ";
        for (idx = 0; idx < max; idx++)
        {
            char c = this->data.at(idx);
            static_cast<char *>(buf)[idx] = c;
            std::cout << c;
        }

        std::cout << std::endl;

        return max;
    }

    quasi_ssize_t RegularFile::write(quasi_off_t offset, const void *buf, quasi_size_t count)
    {
        std::cout << "Writing to file: ";
        for (size_t idx = 0; idx < count; idx++)
        {
            char c = static_cast<const char *>(buf)[idx];
            this->data.push_back(c);
            std::cout << c;
        }

        std::cout << std::endl;

        this->st.st_size += count;

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
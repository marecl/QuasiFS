#include <cstring>
#include <iostream>

#include "include/quasifs_inode_regularfile.h"

namespace QuasiFS
{

    RegularFile::RegularFile()
    {
        st.st_mode = 0000644 | S_IFREG;
        st.st_nlink = 0;
    }

    ssize_t RegularFile::read(off_t offset, void *buf, size_t count)
    {
        size_t idx = 0;
        size_t max = this->data.size();
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

    ssize_t RegularFile::write(off_t offset, const void *buf, size_t count)
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

}
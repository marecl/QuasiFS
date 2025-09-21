#pragma once

#include "quasifs_types.h"
#include "quasifs_inode.h"

namespace QuasiFS
{

    class RegularFile : public Inode
    {
        std::vector<char> data;

    public:
        RegularFile();
        ~RegularFile() = default;

        static file_ptr Create(void)
        {
            return std::make_shared<RegularFile>();
        }

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
        int truncate(off_t length) { return -QUASI_ENOSYS; }
    };

}
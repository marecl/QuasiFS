#pragma once

#include "quasi_types.h"
#include "quasi_inode.h"

namespace QuasiFS
{

    class RegularFile : public Inode
    {
        // std::vector<char> data;
    public:
        RegularFile();
        ~RegularFile() = default;

        ssize_t read(off_t offset, void *buf, size_t count) override;
        ssize_t write(off_t offset, const void *buf, size_t count) override;
    };

}
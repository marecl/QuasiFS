#pragma once
#include "quasi_fs.h"

class NullDevice : public vfs::Inode
{
public:
    bool is_file() const override { return true; }

    ssize_t read(off_t, void *, size_t) override
    {
        return 0; // zawsze EOF
    }

    ssize_t write(off_t, const void *, size_t count) override
    {
        return count; // ignorujemy dane
    }
};
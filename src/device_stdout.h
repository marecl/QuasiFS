#pragma once
#include <iostream>
#include "quasi_fs.h"

class StdoutDevice : public vfs::Inode
{
public:
    bool is_file() const override { return true; }

    ssize_t read(off_t, void *, size_t) override
    {
        return -ENOSYS; // brak read
    }

    ssize_t write(off_t, const void *buf, size_t count) override
    {
        std::string s((const char *)buf, count);
        std::cout << s;
        std::cout.flush();
        return count;
    }
};
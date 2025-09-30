#pragma once

#include "quasifs_types.h"
#include "quasifs_inode.h"

namespace QuasiFS
{

    class RegularFile : public Inode
    {
        std::vector<char> data{};

    public:
        RegularFile();
        ~RegularFile() = default;

        static file_ptr Create(void)
        {
            return std::make_shared<RegularFile>();
        }

        //
        // Working functions
        //
        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
        int truncate(quasi_off_t length);

        //
        // Mock functions
        // Work normally, but don't write data to internal storage
        //
        quasi_ssize_t MockRead(quasi_off_t offset, void *buf, quasi_size_t count);
        quasi_ssize_t MockWrite(quasi_off_t offset, const void *buf, quasi_size_t count);
        int MockTruncate(quasi_off_t length);
    };

}
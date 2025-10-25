// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"
#include "quasifs_inode_file.h"

namespace QuasiFS
{

    class VirtualFile final : public QuasiFile
    {
        std::vector<char> data{};

    public:
        VirtualFile() = default;
        ~VirtualFile() = default;

        //
        // Working functions
        //
        quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) override;
        quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) override;
        int ftruncate(quasi_off_t length)  override;
    };

} // namespace QuasiFS
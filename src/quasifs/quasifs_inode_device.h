// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"
#include "quasifs_inode.h"

namespace QuasiFS
{

    class Device : public Inode
    {

    public:
        Device();
        ~Device() = default;

        static dev_ptr Create(void)
        {
            return std::make_shared<Device>();
        }

        virtual quasi_off_t lseek(quasi_off_t offset, QuasiFS::SeekOrigin origin) override { return -QUASI_ESPIPE; }
        virtual int ftruncate(quasi_off_t length) override { return -QUASI_EINVAL; };
    };

}
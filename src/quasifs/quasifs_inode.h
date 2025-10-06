// INAA License @marecl 2025

#pragma once

#include <vector>

#include "quasi_sys_stat.h"
#include "quasi_errno.h"
#include "quasi_types.h"

namespace QuasiFS
{
    using namespace Stat;

    class Inode
    {
    public:
        Inode() = default;
        virtual ~Inode() = default;

        static inode_ptr Create(void)
        {
            return std::make_shared<Inode>();
        }

        // ioctl(unsigned long op, ...);
        virtual quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) { return -QUASI_ENOSYS; }
        virtual quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) { return -QUASI_ENOSYS; }
        // readv
        // writev
        // preadv

        virtual quasi_off_t lseek(quasi_off_t offset, QuasiFS::SeekOrigin origin) { return -QUASI_ENOSYS; }
        virtual int ftruncate(quasi_off_t length) { return -QUASI_ENOSYS; };
        virtual ssize_t getdents(void *buf, unsigned int nbytes, size_t *basep) { return -QUASI_ENOSYS; };
        virtual int fsync(void) { return -QUASI_ENOSYS; };

        virtual int fstat(quasi_stat_t *stat)
        {
            *stat = st;
            return 0;
        }

        // type helpers
        quasi_mode_t type(void) const { return st.st_mode & QUASI_S_IFMT; }
        bool is_file(void) const { return QUASI_S_ISREG(st.st_mode); }
        bool is_dir(void) const { return QUASI_S_ISDIR(st.st_mode); }
        bool is_link(void) const { return QUASI_S_ISLNK(st.st_mode); }
        bool is_char(void) const { return QUASI_S_ISCHR(st.st_mode); }

        bool CanRead(void) { return this->st.st_mode & (QUASI_S_IRUSR | QUASI_S_IRGRP | QUASI_S_IROTH); }
        bool CanWrite(void) { return this->st.st_mode & (QUASI_S_IWUSR | QUASI_S_IWGRP | QUASI_S_IWOTH); }
        bool CanExecute(void) { return this->st.st_mode & (QUASI_S_IXUSR | QUASI_S_IXGRP | QUASI_S_IXOTH); }

        fileno_t GetFileno(void) { return this->fileno; }
        fileno_t SetFileno(fileno_t fileno)
        {
            this->fileno = fileno;
            return fileno;
        };

        fileno_t fileno{-1};
        quasi_stat_t st{};
    };

}
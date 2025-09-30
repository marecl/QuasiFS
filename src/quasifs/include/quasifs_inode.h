#pragma once

#include <vector>
#include <sys/stat.h>

#include "quasi_errno.h"
#include "quasifs_types.h"

namespace QuasiFS
{
    class Inode
    {
    public:
        Inode() = default;
        virtual ~Inode() = default;

        static inode_ptr Create(void)
        {
            return std::make_shared<Inode>();
        }

        // file-like
        virtual quasi_ssize_t read(quasi_off_t offset, void *buf, quasi_size_t count) { return -QUASI_ENOSYS; }
        virtual quasi_ssize_t write(quasi_off_t offset, const void *buf, quasi_size_t count) { return -QUASI_ENOSYS; }

        // metadata
        virtual quasi_stat_t getattr(void) { return st; }

        // type helpers
        quasi_mode_t type(void) const { return st.st_mode & S_IFMT; }
        bool is_file(void) const { return type() == S_IFREG; }
        bool is_dir(void) const { return type() == S_IFDIR; }
        bool is_link(void) const { return type() == S_IFLNK; }
        bool is_char(void) const { return type() == S_IFCHR; }

        bool CanRead(void) { return this->st.st_mode & (S_IRUSR | S_IRGRP | S_IROTH); }
        bool CanWrite(void) { return this->st.st_mode & (S_IWUSR | S_IWGRP | S_IWOTH); }
        bool CanExecute(void) { return this->st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH); }

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
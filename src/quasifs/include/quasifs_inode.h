#pragma once

#include <vector>
#include <sys/stat.h>

#include "quasi_errno.h"
#include "quasifs_types.h"
#include "host_io.h"

namespace QuasiFS
{

    // Base Inode: domyślnie -QUASI_ENOSYS (niezaimplementowane)
    class Inode : std::enable_shared_from_this<Inode>
    {
    public:
        Inode() = default;
        Inode(fs::path &path) : host_path(path) {};
        virtual ~Inode() = default;

        static inode_ptr Create(void)
        {
            return std::make_shared<Inode>();
        }

        // file-like
        virtual ssize_t read(off_t offset, void *buf, size_t count) { return -QUASI_ENOSYS; }
        virtual ssize_t write(off_t offset, const void *buf, size_t count) { return -QUASI_ENOSYS; }

        // metadata
        virtual Stat getattr() { return st; }

        // type helpers
        mode_t type() const { return st.st_mode & S_IFMT; }
        bool is_file() const { return type() == S_IFREG; }
        bool is_dir() const { return type() == S_IFDIR; }
        bool is_link() const { return type() == S_IFLNK; }
        bool is_char() const { return type() == S_IFCHR; }

        fileno_t GetFileno(void) { return this->fileno; }
        fileno_t SetFileno(fileno_t fileno)
        {
            this->fileno = fileno;
            return fileno;
        };

        fileno_t fileno{-1};
        Stat st{};

        HostVIO io_driver;
        fs::path host_path{};
    };

}
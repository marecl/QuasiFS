#pragma once

#include <vector>
#include <sys/stat.h>
#include "quasi_types.h"

namespace QuasiFS
{

    // Base Inode: domyślnie -ENOSYS (niezaimplementowane)
    class Inode : std::enable_shared_from_this<Inode>
    {
    public:
        Inode() = default;
        virtual ~Inode() = default;

        template <typename T, typename... Args>
        static std::shared_ptr<T> Create(Args &&...args)
        {
            static_assert(std::is_base_of<Inode, T>::value, "T must derive from Inode");
            return std::make_shared<T>(std::forward<Args>(args)...);
        }

        // file-like
        virtual ssize_t read(off_t /*offset*/, void * /*buf*/, size_t /*count*/) { return -ENOSYS; }
        virtual ssize_t write(off_t /*offset*/, const void * /*buf*/, size_t /*count*/) { return -ENOSYS; }
        virtual int truncate(off_t /*length*/) { return -ENOSYS; }

        // metadata
        virtual Stat getattr() { return st; }

        // type helpers
        int type() const { return st.mode & S_IFMT; }
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
    };

}
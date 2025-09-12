#pragma once

#include <vector>
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
            return std::make_shared<T>(std::forward<Args>(args)...);
        }

        // file-like
        virtual ssize_t read(off_t /*offset*/, void * /*buf*/, size_t /*count*/) { return -ENOSYS; }
        virtual ssize_t write(off_t /*offset*/, const void * /*buf*/, size_t /*count*/) { return -ENOSYS; }
        virtual int truncate(off_t /*length*/) { return -ENOSYS; }

        // dir-like
        virtual inode_ptr lookup(const std::string & /*name*/) { return nullptr; }
        virtual int link(inode_ptr /*inode*/, const std::string & /*name*/) { return -ENOSYS; }
        virtual int unlink(const std::string & /*name*/) { return -ENOSYS; }
        virtual std::vector<std::string> list() { return {}; }
        virtual int mkdir(const std::string & /*name*/, dir_ptr inode /* directory */) { return -ENOSYS; }

        // metadata
        virtual Stat getattr() { return st; }

        // type helpers
        virtual bool is_dir() const { return false; }
        virtual bool is_file() const { return false; }

        fileno_t GetFileno(void) { return this->fileno; }
        fileno_t SetFileno(fileno_t fileno)
        {
            this->fileno = fileno;
            return fileno;
        };

        fileno_t fileno = -1;
        Stat st;
    };

}
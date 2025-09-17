#include <map>
#include <string>

#include "include/quasifs_inode_directory.h"

namespace QuasiFS
{

    Directory::Directory()
    {
        st.st_mode = 0000755 | S_IFDIR;
        st.st_nlink = 0;
    }

    inode_ptr Directory::lookup(const std::string &name)
    {
        auto it = entries.find(name);
        if (it == entries.end())
            return nullptr;
        return it->second;
    }

    int Directory::link(const std::string &name, inode_ptr child)
    {
        if (nullptr == child)
            return -ENOENT;

        if (child->is_dir())
            // CAN'T link a directory
            return -EINVAL;

        if (entries.count(name))
            return -EEXIST;
        entries[name] = child;
        if (!child->is_link())
            child->st.st_nlink++;
        return 0;
    }

    int Directory::unlink(const std::string &name)
    {
        auto it = entries.find(name);
        if (it == entries.end())
            return -ENOENT;

        inode_ptr target = it->second;
        // if directory and not empty -> EBUSY or ENOTEMPTY
        if (target->is_dir())
        {
            dir_ptr dir = std::static_pointer_cast<Directory>(target);
            auto children = dir->list();
            children.erase(std::remove(children.begin(), children.end(), "."), children.end());
            children.erase(std::remove(children.begin(), children.end(), ".."), children.end());
            if (!children.empty())
                return -ENOTEMPTY;
        }

        target->st.st_nlink--;
        entries.erase(it);
        return 0;
    }

    std::vector<std::string> Directory::list()
    {
        std::vector<std::string> r;
        for (auto &p : entries)
            r.push_back(p.first);
        return r;
    }

    int Directory::mkdir(const std::string &name, dir_ptr child)
    {
        if (entries.count(name))
            return -EEXIST;
        entries[name] = child;
        child->st.st_nlink++;
        return 0;
    }

    Stat Directory::getattr()
    {
        // st_size: arbitrary number of entries * 32
        st.st_size = entries.size() * 32;
        return st;
    }

}
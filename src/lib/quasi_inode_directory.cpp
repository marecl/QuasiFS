#include <map>
#include <string>

#include "include/quasi_inode_directory.h"

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

    int Directory::link(inode_ptr inode, const std::string &name)
    {
        if (nullptr == inode)
            return -ENOENT;

        if (inode->is_dir())
            // CAN'T link a directory
            return -EINVAL;

        if (entries.count(name))
            return -EEXIST;
        entries[name] = inode;
        if (!inode->is_link())
            inode->st.st_nlink++;
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

    int Directory::mkdir(dir_ptr dir, const std::string &name)
    {
        if (entries.count(name))
            return -EEXIST;
        entries[name] = dir;
        dir->st.st_nlink++;
        return 0;
    }

    Stat Directory::getattr()
    {
        // st_size: arbitrary number of entries * 32
        st.st_size = entries.size() * 32;
        return st;
    }

}
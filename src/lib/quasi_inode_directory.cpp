#include <map>
#include <string>

#include "include/quasi_types.h"

#include "include/quasi_inode.h"
#include "include/quasi_inode_directory.h"

namespace QuasiFS
{

    Directory::Directory()
    {
        st.mode = 0000755 | S_IFDIR;
        st.nlink = 0;
    }

    inode_ptr Directory::lookup(const std::string &name)
    {
        // if (mounted_root)
        // {
        //     return mounted_root->lookup(name);
        // }
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
            return -EPERM;

        // if (mounted_root)
        // {
        //     return mounted_root->link(inode, name);
        // }

        if (entries.count(name))
            return -EEXIST;
        entries[name] = inode;
        if (!inode->is_link())
            inode->st.nlink++;
        return 0;
    }

    int Directory::unlink(const std::string &name)
    {
        // if (mounted_root)
        // {
        //     return mounted_root->unlink(name);
        // }

        auto it = entries.find(name);
        if (it == entries.end())
            return -ENOENT;
        // if directory and not empty -> EBUSY or ENOTEMPTY
        if (it->second->is_dir())
        {
            auto children = it->second->list();
            children.erase(std::remove(children.begin(), children.end(), "."), children.end());
            children.erase(std::remove(children.begin(), children.end(), ".."), children.end());
            if (!children.empty())
                return -ENOTEMPTY;
        }
        auto nlink = &it->second->st.nlink;
        *nlink = (it->second->st.nlink > 0) ? it->second->st.nlink - 1 : 0;
        entries.erase(it);
        return 0;
    }

    std::vector<std::string> Directory::list()
    {
        // if (mounted_root)
        // {
        //     return mounted_root->list();
        // }
        std::vector<std::string> r;
        for (auto &p : entries)
            r.push_back(p.first);
        return r;
    }

    int Directory::mkdir(const std::string &name, dir_ptr inode)
    {
        // if (mounted_root)
        // {
        //     return mounted_root->mkdir(name, inode);
        // }
        if (entries.count(name))
            return -EEXIST;
        entries[name] = inode;
        inode->st.nlink++;
        return 0;
    }

    Stat Directory::getattr()
    {
        // size: arbitrary number of entries * 32
        st.size = entries.size() * 32;
        return st;
    }

}
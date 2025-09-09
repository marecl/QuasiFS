
#pragma once

#include <cstring>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <chrono>
#include <system_error>
#include <filesystem>

// Errno constants
#include <errno.h>
#include <stdint.h>

#define STUB(x)                                  \
    {                                            \
        std::cout << "STUB: " << x << std::endl; \
    }

namespace vfs
{
    namespace fs = std::filesystem;
    using fileno_t = int64_t;
    using time_point = std::chrono::system_clock::time_point;
    using blkid_t = uint64_t;

    // Forward
    class Inode;
    using inode_ptr = std::shared_ptr<Inode>;
    class FileSystem;
    using fs_ptr = std::shared_ptr<FileSystem>;

    struct Stat
    {
        uint64_t size = 0;
        uint64_t blocks = 0;
        uint32_t mode = 0; // posix mode bits
        uint64_t ino = 0;
        uint32_t nlink = 0;
        uint64_t dev = 0;
        time_point atime, mtime, ctime;
    };

    // Base Inode: domyślnie -ENOSYS (niezaimplementowane)
    class Inode : std::enable_shared_from_this<Inode>
    {
    public:
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
        virtual int link(const std::string & /*name*/, inode_ptr /*inode*/) { return -ENOSYS; }
        virtual int unlink(const std::string & /*name*/) { return -ENOSYS; }
        virtual std::vector<std::string> list() { return {}; }
        virtual int mkdir(const std::string & /*name*/) { return -ENOSYS; }

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

    class HostMount
    {
    protected:
        HostMount() = default;
        ~HostMount() = default;
        void SetHostPath(fs::path path)
        {
            this->host_path = path;
        };
        fs::path GetHostPath(void)
        {
            return this->host_path;
        };

    private:
        fs::path host_path{};
    };

    class RegularFile : public Inode,
                        public HostMount
    {
        // std::vector<char> data;
    public:
        RegularFile() { st.mode = 0100644; /* regular file */ }
        bool is_file() const override { return true; }

        ssize_t read(off_t offset, void *buf, size_t count) override
        {
            STUB("RegularFile: read()");
            memcpy(buf, "XD\0", 3);
            return 3;
        }

        ssize_t write(off_t offset, const void *buf, size_t count) override
        {
            STUB("RegularFile: write()");
            return count;
        }
    };

    // Directory
    struct Directory : public Inode, public HostMount
    {
        std::map<std::string, inode_ptr> entries{};
        // if mounted_fs_root != nullptr, this directory is a mountpoint and resolution should go to mounted root
        inode_ptr mounted_root = nullptr;

        Directory() { st.mode = 0040755; /* dir */ }
        bool is_dir() const override { return true; }

        inode_ptr lookup(const std::string &name) override
        {
            if (mounted_root)
            {
                // if root is mounted here and name refers to child of mounted root, delegate
                return mounted_root->lookup(name);
            }
            auto it = entries.find(name);
            if (it == entries.end())
                return nullptr;
            return it->second;
        }

        int link(const std::string &name, inode_ptr inode) override
        {
            if (entries.count(name))
                return -EEXIST;
            entries[name] = inode;
            inode->st.nlink++;
            return 0;
        }

        int unlink(const std::string &name) override
        {
            if (mounted_root)
            {
                // unlink in mounted fs
                // delegate by name to mounted root
                return mounted_root->unlink(name);
            }
            auto it = entries.find(name);
            if (it == entries.end())
                return -ENOENT;
            // if directory and not empty -> EBUSY or ENOTEMPTY
            if (it->second->is_dir())
            {
                auto children = it->second->list();
                if (!children.empty())
                    return -ENOTEMPTY;
            }
            it->second->st.nlink = (it->second->st.nlink > 0) ? it->second->st.nlink - 1 : 0;
            entries.erase(it);
            return 0;
        }

        std::vector<std::string> list() override
        {
            if (mounted_root)
                return mounted_root->list();
            std::vector<std::string> r;
            for (auto &p : entries)
                r.push_back(p.first);
            return r;
        }

        int mkdir(const std::string &name) override
        {
            if (mounted_root)
                return mounted_root->mkdir(name);
            if (entries.count(name))
                return -EEXIST;
            inode_ptr d = std::make_shared<Directory>();
            entries[name] = d;
            d->st.nlink = 1;
            return 0;
        }

        Stat getattr() override
        {
            // size: arbitrary number of entries * 32
            st.size = entries.size() * 32;
            return st;
        }
    };

    // FileSystem interface
    class FileSystem
    {
    public:
        FileSystem() : block_id(next_block_id++)
        {
            this->root = std::make_shared<Directory>();
            IndexInode(this->root);
            inode_table[this->root->GetFileno()] = this->root;
        };
        ~FileSystem() = default;

        static fs_ptr Create(void)
        {
            return std::make_shared<FileSystem>();
        }

        inode_ptr GetRoot(void) { return this->root; };
        blkid_t GetBlkid(void) { return this->block_id; };

        void IndexInode(inode_ptr node)
        {
            fileno_t node_fileno = node->GetFileno();
            if (!node)
                return;
            if (node_fileno == -1)
                node_fileno = node->SetFileno(this->NextFileno());

            inode_table[node_fileno] = node;
            if (node->is_dir())
            {
                auto dir = std::static_pointer_cast<Directory>(node);
                for (auto &kv : dir->entries)
                    IndexInode(kv.second);
                if (dir->mounted_root)
                    IndexInode(dir->mounted_root);
            }
            node->st.nlink = 1;
        }

    private:
        fileno_t NextFileno(void) { return this->next_fileno++; };
        // wlasne inode
        std::unordered_map<fileno_t, inode_ptr> inode_table;

        inode_ptr root;
        fileno_t next_fileno = 2;
        const blkid_t block_id;

        static inline blkid_t next_block_id = 1;
    };

    // Very small VFS manager: path resolution, mount, create/unlink
    class VFS
    {
        inode_ptr root;
        // fileno of a directory is a mountpoint
        std::unordered_map<fileno_t, fs_ptr> fs_table{};
        std::unordered_map<fileno_t, blkid_t> inode_table{};
        // std::vector<int,File*> open_handles;

    public:
        VFS()
        {
            auto mfs = FileSystem::Create();
            this->root = mfs->GetRoot();
        }

        inode_ptr GetRoot() { return root; }

        // resolve path into (parent_dir, leaf_name, inode)
        struct Resolved
        {
            inode_ptr mountpoint;
            inode_ptr parent;
            inode_ptr node; // nullptr if doesn't exist
            std::string leaf;
        };

        Resolved resolve(const fs::path &path)
        {
            Resolved res;
            if (path.empty() || path.is_relative())
                return res; // tylko ścieżki absolutne

            inode_ptr mntpoint = this->root;
            inode_ptr current_inode = this->root;
            inode_ptr parent_inode = nullptr;
            fs::path leaf{};

            for (auto it = path.begin(); it != path.end(); ++it)
            {
                leaf = *it;
                if (leaf.empty() || leaf == "/")
                    continue;

                res.leaf = leaf;

                auto dir = std::dynamic_pointer_cast<Directory>(current_inode);
                if (!dir)
                {
                    // próbujemy iść w dół w pliku
                    res.parent = parent_inode;
                    res.node = nullptr;
                    return res;
                }

                // jeśli katalog jest mountpointem, delegujemy do mounted_root
                if (dir->mounted_root)
                {
                    if (!current_inode)
                    {
                        res.parent = nullptr;
                        res.node = nullptr;
                        return res;
                    }
                    mntpoint = dir->mounted_root;
                }

                parent_inode = current_inode;
                current_inode = dir->lookup(leaf);
                if (std::next(it) == path.end())
                {
                    // ostatni element ścieżki
                    res.parent = parent_inode;
                    res.leaf = leaf;
                    res.node = current_inode;
                    return res;
                }

                if (!current_inode)
                {
                    // ścieżka nie istnieje
                    res.parent = nullptr;
                    res.node = nullptr;
                    return res;
                }
            }

            // jeśli ścieżka to "/", zwracamy root
            res.parent = nullptr;
            res.leaf = "/";
            res.node = root;
            return res;
        }

        // create file at path (creates entry in parent dir). returns 0 or negative errno
        int create(const fs::path &path)
        {
            auto res = resolve(path);
            if (!res.parent)
                return -ENOENT;
            if (res.node)
                return -EEXIST;
            if (!res.parent->is_dir())
                return -ENOTDIR;

            auto fsblk_node = res.mountpoint;

            auto mntpoint = this->fs_table.find(fsblk_node->GetFileno());
            if (mntpoint == this->fs_table.end())
                return -ENOENT;

            fs_ptr fsblk = mntpoint->second;

            auto dir = std::static_pointer_cast<Directory>(res.parent);
            inode_ptr file = std::make_shared<RegularFile>();
            fsblk->IndexInode(file);
            return dir->link(res.leaf, file);
        }

        int mkdir(const std::string &path)
        {
            auto res = resolve(path);
            if (!res.parent)
                return -ENOENT;
            if (res.node)
                return -EEXIST;
            if (!res.parent->is_dir())
                return -ENOTDIR;

            auto fsblk_node = res.mountpoint;

            auto mntpoint = this->fs_table.find(fsblk_node->GetFileno());
            if (mntpoint == this->fs_table.end())
                return -ENOENT;

            fs_ptr fsblk = mntpoint->second;

            auto dir = std::static_pointer_cast<Directory>(res.parent);
            fsblk->IndexInode(file);
            return dir->mkdir(res.leaf);
        }

        int unlink(const std::string &path)
        {
            auto res = resolve(path);
            if (!res.parent)
                return -ENOENT;
            if (!res.node)
                return -ENOENT;
            if (!res.parent->is_dir())
                return -ENOTDIR;
            auto dir = std::static_pointer_cast<Directory>(res.parent);
            return dir->unlink(res.leaf);
        }

        // mount fs at path (target must exist and be directory)
        int mount(const std::string &target_path, fs_ptr fs)
        {
            auto res = resolve(target_path);
            if (!res.node)
                return -ENOENT;
            if (!res.node->is_dir())
                return -ENOTDIR;
            if (res.mountpoint)
                return -EEXIST;

            auto dir = std::static_pointer_cast<Directory>(res.node);
            dir->mounted_root = fs->GetRoot();
            this->fs_table[fs->GetBlkid()] = fs;
            return 0;
        }

    }; // class VFS

} // namespace vfs

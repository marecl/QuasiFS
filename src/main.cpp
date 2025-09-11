#include <iostream>
#include <string>
#include <memory>
#include "quasi_fs.h"

using namespace vfs;

void printTree(const std::shared_ptr<Inode> &node, const std::string &name, int depth = 0)
{
    std::string indent(depth * 2, ' ');
    std::string type = node->is_dir() ? "dir" : (node->is_file() ? "file" : "other");

    std::cout << indent << name << " [fileno=" << node->fileno << ", " << type << "]\n";

    if (node->is_dir())
    {
        auto dir = std::dynamic_pointer_cast<Directory>(node);
        if (dir->mounted_root)
        {
            printTree(dir->mounted_root, "(mount)", depth + 1);
        }
        else
        {
            for (auto &[childName, child] : dir->entries)
            {
                printTree(child, childName, depth + 1);
            }
        }
    }
}

int main()
{
    VFS v;

    // mkdir /data i /var
    auto datafs = FileSystem::Create();
    auto devfs = FileSystem::Create();
    auto varfs = FileSystem::Create();

    // datafs->mkdir(datafs->GetRoot(), "XD");
    // datafs->touch(datafs->GetRoot(), "lol.lol");

    // printTree(datafs->GetRoot(), "/data");
    v.mkdir("/data");
    v.mkdir("/dev");
    v.mkdir("/var");
    v.mkdir("/system");

    v.touch("/system/123");

    v.mount("/data", datafs);
    v.mount("/dev", devfs);
    v.mount("/var", varfs);

    v.mkdir("/data/rtyrtyrty");
    v.mkdir("/data/qweqwe");
    v.touch("/data/XDFXDXDXD");

    v.mkdir("/var/asdasd");
    v.mkdir("/var/asdasd/asdasd");
    v.touch("/var/asdasd/asdasd/hihi");


    printTree(v.GetRoot(), "/", 0);

    auto sysfs = FileSystem::Create();
    v.mount("/system", sysfs);
    v.touch("/system/456");

    // wydruk drzewa
    std::cout << "=== VFS tree ===\n";
    printTree(v.GetRoot(), "/", 0);

    v.unmount("/system");
    printTree(v.GetRoot(), "/", 0);

    return 0;
}
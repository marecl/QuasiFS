#include <iostream>
#include <string>
#include <memory>
#include "quasi_fs.h"
#include "device_null.h"
#include "device_stdout.h"

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

    v.mount("/data", datafs);
    v.mount("/dev", devfs);
    v.mount("/var", varfs);

    // mount /dev
    auto devdir = std::dynamic_pointer_cast<Directory>(devfs->GetRoot());
    devdir->link("stdout", std::make_shared<StdoutDevice>());
    devdir->link("null", std::make_shared<NullDevice>());

    // dodaj plik do /var
    auto varDir = std::dynamic_pointer_cast<Directory>(varfs->GetRoot());
    varDir->link("log.txt", std::make_shared<RegularFile>());

    auto dataDir = std::dynamic_pointer_cast<Directory>(datafs->GetRoot());
    dataDir->link("XD", std::make_shared<Directory>());
    dataDir->link("hihi.exe", std::make_shared<Directory>());
    dataDir->link("chuj.log", std::make_shared<RegularFile>());

    // wydruk drzewa
    std::cout << "=== VFS tree ===\n";
    printTree(v.resolve("/").node, "/", 0);

    v.create("/dev/chuj");

    return 0;
}
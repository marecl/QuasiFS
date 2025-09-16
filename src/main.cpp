#include <iostream>
#include <string>
#include <memory>
#include <format>
#include "lib/include/quasi_fs.h"
#include "lib/include/quasi_inode.h"
#include "lib/include/quasi_inode_directory.h"
#include "lib/include/quasi_inode_regularfile.h"
#include "lib/include/quasi_partition.h"

#include "log.h"
#include "tests.h"

using namespace QuasiFS;

void printTree(const inode_ptr &node, const std::string &name, int depth = 0)
{
    std::string depEnt = "\t";
    for (uint8_t q = 0; q < depth; q++)
    {
        depEnt = depEnt + "|--";
    }
    if (depth > 0)
        depEnt[depEnt.length() - 1] = '>';

    std::string type = node->is_dir() ? "[DIR]\t" : (node->is_file() ? "[FIL]\t" : "[OTH]\t");

    if (!name.empty())
        std::cout << type << std::format("[{:3d}] ", node->fileno) << depEnt << name << std::endl;
    else
        depth--;

    if (name == ".")
        return;
    if (name == "..")
        return;

    if (node->is_dir())
    {
        auto dir = std::dynamic_pointer_cast<Directory>(node);
        if (dir->mounted_root)
        {
            printTree(dir->mounted_root, "", depth + 1);
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

    partition_ptr p = Partition::Create();

    // // mkdir /data i /var
    // auto datafs = Partition::Create();
    // auto devfs = Partition::Create();
    // auto varfs = Partition::Create();

    // // datafs->mkdir(datafs->GetRoot(), "XD");
    // // datafs->touch(datafs->GetRoot(), "lol.lol");

    // // printTree(datafs->GetRoot(), "/data");
    // v.mkdir("/data");
    // v.mkdir("/dev");
    // v.mkdir("/var");
    // v.mkdir("/system");

    // v.touch("/system/123");

    // v.mount("/data", datafs);
    // v.mount("/dev", devfs);
    // v.mount("/var", varfs);

    // v.mkdir("/data/rtyrtyrty");
    // v.mkdir("/data/qweqwe");
    // v.touch("/data/XDFXDXDXD");

    // v.mkdir("/var/asdasd");
    // v.mkdir("/var/asdasd/asdasd");
    // v.touch("/var/asdasd/asdasd/hihi");

    // printTree(v.GetRoot(), "/", 0);

    // auto sysfs = Partition::Create();
    // v.mount("/system", sysfs);
    // v.touch("/system/456");

    // // wydruk drzewa
    // std::cout << "=== QFS tree ===\n";
    // printTree(v.GetRoot(), "/", 0);

    // v.unmount("/system");

    QFS v;
    v.mkdir("/XD");
    // Test(v);

    Log("<<<< PRINTING FS TREE >>>>");
    printTree(v.GetRoot(), "/", 0);

    return 0;
}
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
    Test(v);

    Log("<<<< PRINTING FS TREE >>>>");
    printTree(v.GetRoot(), "/", 0);

    return 0;
}
#include <iostream>
#include <string>
#include <memory>
#include <format>
#include "quasifs/include/quasifs.h"
#include "quasifs/include/quasifs_inode.h"
#include "quasifs/include/quasifs_inode_directory.h"
#include "quasifs/include/quasifs_inode_regularfile.h"
#include "quasifs/include/quasifs_partition.h"

#include "dev/include/dev_std.h"

#include "log.h"
#include "tests.h"

using namespace QuasiFS;

int main()
{
    QFS v;
    Test(v);

    // QFS vv;
    // vv.mkdir("/dev");

    // auto stdo = std::make_shared<Devices::DevStdout>();
    // auto stdi = std::make_shared<Devices::DevStdin>();
    // auto stde = std::make_shared<Devices::DevStderr>();
    // vv.touch("/dev", "stdout", stdo);
    // vv.touch("/dev", "stdin", stdi);
    // vv.touch("/dev", "stderr", stde);
    // Log("<<<< PRINTING FS TREE >>>>");
    // printTree(vv.GetRoot(), "/", 0);

    return 0;
}
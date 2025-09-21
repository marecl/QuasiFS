#include "quasifs/include/quasifs.h"
#include "quasifs/include/quasifs_inode.h"
#include "quasifs/include/quasifs_inode_directory.h"
#include "quasifs/include/quasifs_inode_regularfile.h"
#include "quasifs/include/quasifs_partition.h"

#include "dev/include/dev_std.h"

#include "log.h"
// #include "tests.h"
#include <filesystem>
using namespace QuasiFS;

int main()
{
    QFS v;

    Log("{}", std::filesystem::absolute(".").string());

    v.MKDir("/host");
    v.MapHost("/host", "./");
    v.MKDir("/host/hehe");
    v.Creat("/host/hehe/qweqwe.txt");
    v.Creat("/host/zxczxc.txt");

    // Test(v);

    // QFS vv;
    // vv.MKDir("/dev");

    // auto stdo = std::make_shared<Devices::DevStdout>();
    // auto stdi = std::make_shared<Devices::DevStdin>();
    // auto stde = std::make_shared<Devices::DevStderr>();
    // vv.Touch("/dev", "stdout", stdo);
    // vv.Touch("/dev", "stdin", stdi);
    // vv.Touch("/dev", "stderr", stde);
    // Log("<<<< PRINTING FS TREE >>>>");
    // printTree(vv.GetRoot(), "/", 0);

    return 0;
}
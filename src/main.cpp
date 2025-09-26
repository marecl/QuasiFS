#include "quasifs/include/quasifs.h"
#include "quasifs/include/quasifs_inode.h"
#include "quasifs/include/quasifs_inode_directory.h"
#include "quasifs/include/quasifs_inode_regularfile.h"
#include "quasifs/include/quasifs_partition.h"

#include "dev/include/dev_std.h"

#include "log.h"
#include "tests.h"
#include <filesystem>
using namespace QuasiFS;

int main()
{

  // // Virtual test
  // QFS v;
  // Test(v);

  // // Host-bound test

  // prepare test
  try
  {
    fs::remove_all(fs::absolute("test_root"));
  }
  catch (...)
  {
  }

  fs::path test_root_dir = fs::absolute("test_root");
  fs::create_directory(test_root_dir);

  QFS vh(test_root_dir);
  // vh.SyncHost();

  // vh.MKDir("/mmm");
  // vh.Creat("/mmm/XD.txt");
  // partition_ptr part = Partition::Create();
  // vh.Mount("/mmm", part);
  // vh.Creat("/mmm/XDD.txt");

  // // vh.SyncHost();

  int status;
  status = vh.MKDir("/host");
  status = vh.Creat("/hostfile");

  status = vh.MKDir("/host/hehe");
  // status = vh.MKDir("/host/hehe/hihihi");
  status = vh.Creat("/host/hehe/qweqwe.txt");
  // status = vh.Creat("/host/hehe/hihihi/zxczxc.txt");
  // status = vh.Creat("/host/hehe/hihihi/../../../..//zxczxc.txt");

  printTree(vh.GetRoot(), "/", 0);

  Test(vh);

  // end test
  // fs::remove_all(test_root_dir);

  // QFS vv;
  // vv.MKDir("/dev");

  // auto stdo = std::make_shared<Devices::DevStdout>();
  // auto stdi = std::make_shared<Devices::DevStdin>();
  // auto stde = std::make_shared<Devices::DevStderr>();
  // vv.Touch("/dev", "stdout", stdo);
  // vv.Touch("/dev", "stdin", stdi);
  // vv.Touch("/dev", "stderr", stde);
  // Log("<<<< PRINTING FS TREE >>>>");
  printTree(vh.GetRoot(), "/", 0);

  return 0;
}
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
  fs::path test_root_dir = fs::absolute("test_root");

  try
  {
    fs::remove_all(test_root_dir);
  }
  catch (...)
  {
  }

  fs::create_directory(test_root_dir);

  QFS vh(test_root_dir);
  // QFS vh;

  // vh.SyncHost();

  printTree(vh.GetRoot(), "/", 0);

  Test(vh);

  Log("<<<< PRINTING FS TREE >>>>");
  printTree(vh.GetRoot(), "/", 0);

  return 0;
}
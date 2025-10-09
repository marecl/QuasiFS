// INAA License @marecl 2025

#include "quasifs/quasifs.h"

#include "log.h"
#include "tests.h"

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
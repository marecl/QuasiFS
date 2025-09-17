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
    QFS v;
    Test(v);

    Log("<<<< PRINTING FS TREE >>>>");
    printTree(v.GetRoot(), "/", 0);

    return 0;
}
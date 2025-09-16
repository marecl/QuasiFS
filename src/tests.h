#pragma once

#include "lib/include/quasi_fs.h"
#include "log.h"

using namespace QuasiFS;

void TestTouchUnlinkFile(QFS &qfs);
void TestMkRmdir(QFS &qfs);
void TestMount(QFS &qfs);
void TestMountFileRetention(QFS &qfs);
void TestStLink(QFS &qfs)
{
    LogTest("File link counter");

    qfs.touch("/file");

    Resolved r = qfs.resolve("/file");
    file_ptr f = std::reinterpret_pointer_cast<RegularFile>(r.node);

    if (f->st.nlink != 1)
    {
        LogError("New file link {} != 1", f->st.nlink);
    }

    // r.mountpoint->rmInode()
}

void Test(QFS &qfs)
{
    Log("");
    Log("QuasiFS Test Suite");
    Log("");

    TestTouchUnlinkFile(qfs);
    TestMkRmdir(qfs);
    TestMount(qfs);
    TestMountFileRetention(qfs);

    Log("");
    Log("Tests complete");
    Log("");
}

void TestTouchUnlinkFile(QFS &qfs)
{
    LogTest("Create a file");

    if (0 != qfs.touch("/testfile"))
    {
        LogError("touch failed");
    }

    Resolved r = qfs.resolve("/testfile");

    if (r.parent != qfs.GetRoot())
    {
        LogError("Wrong parent");
    }

    if (!r.node)
    {
        LogError("Not created");
    }

    if (0 != qfs.unlink("/testfile"))
    {
        LogError("rmdir failed");
    }

    r = qfs.resolve("/testfile");

    if (r.node)
    {
        LogError("Didn't remove file");
    }
}

void TestMkRmdir(QFS &qfs)
{
    LogTest("Create a directory");

    if (0 != qfs.mkdir("/testdir"))
    {
        LogError("mkdir failed");
    }

    Resolved r = qfs.resolve("/testdir");

    if (r.parent != qfs.GetRoot())
    {
        LogError("Wrong parent");
    }

    if (!r.node)
    {
        LogError("Not created");
    }

    if (0 != qfs.rmdir("/testdir"))
    {
        LogError("rmdir failed");
    }

    r = qfs.resolve("/testdir");

    if (r.node)
    {
        LogError("Didn't remove dir");
    }
}

void TestMount(QFS &qfs)
{
    LogTest("Mount");

    qfs.mkdir("/dummy");
    qfs.mkdir("/dummy/mount");

    auto part = Partition::Create();

    if (nullptr == part)
    {
        LogError("Can't create partition object");
    }

    Resolved r = qfs.resolve("/dummy/mount");

    if (nullptr == r.node)
    {
        LogError("Pre-mount dir not found");
        return;
    }

    auto self_fileno = r.node->lookup(".")->GetFileno();
    auto parent_fileno = r.node->lookup("..")->GetFileno();
    auto parent_from_root = r.parent->lookup("mount")->GetFileno();

    Log("Pre-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root, parent_fileno, self_fileno);

    if (0 != qfs.mount("/dummy/mount", part))
    {
        LogError("Can't mount");
    }

    r = qfs.resolve("/dummy/mount");

    if (nullptr == r.node)
    {
        LogError("Post-mount dir not found");
        return;
    }

    if (r.mountpoint != qfs.GetRootFS())
    {
        LogError("Bad partition");
    }

    if (r.parent != qfs.resolve("/dummy").node)
    {
        LogError("Bad parent");
    }

    auto self_fileno_mount = r.node->lookup(".")->GetFileno();
    auto parent_fileno_mount = r.node->lookup("..")->GetFileno();
    auto parent_from_root_mount = r.parent->lookup("mount")->GetFileno();

    Log("Post-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root_mount, parent_fileno_mount, self_fileno_mount);

    if (self_fileno_mount != 2)
    {
        LogError("Mountpoint root fileno isn't 2");
    }

    if (parent_fileno_mount != 2)
    {
        LogError("Mountpoint root fileno isn't 2");
    }

    if (self_fileno_mount != parent_fileno_mount)
    {
        LogError("Self and parent fileno of a mountpoint is not equal");
    }

    if (self_fileno == self_fileno_mount)
    {
        LogError("Mounted fileno is the same as regular dir (mount failed)");
    }

    if (parent_fileno == parent_fileno_mount)
    {
        LogError("Mounted fileno is the same as regular dir (mount failed)");
    }

    dir_ptr mounted_dir = std::reinterpret_pointer_cast<Directory>(r.node)->mounted_root;
    if (mounted_dir != part->GetRoot())
    {
        LogError("Bad node");
    }

    if (0 != qfs.unmount("/dummy/mount"))
    {
        LogError("Can't unmount");
    }

    qfs.rmdir("/dummy/mount");
    qfs.rmdir("/dummy");
}

void TestMountFileRetention(QFS &qfs)
{
    LogTest("File retention/substitution on remount");

    auto part = Partition::Create();

    qfs.mkdir("/mount");

    qfs.touch("/mount/dummy1"); // dummy files to shift fileno
    qfs.touch("/mount/dummy2");
    qfs.touch("/mount/dummy3");
    qfs.touch("/mount/dummy4");
    qfs.touch("/mount/testfile");
    qfs.mkdir("/mount/testdir");

    Resolved rf = qfs.resolve("/mount/testfile");
    Resolved rd = qfs.resolve("/mount/testdir");

    if (!rf.node)
    {
        LogError("Test file not created");
        return;
    }
    if (!rd.node)
    {
        LogError("Test directory not created");
        return;
    }

    auto ffileno = rf.node->GetFileno();
    auto dfileno = rd.node->GetFileno();
    Log("Pre-mount file fileno: {}", ffileno);
    Log("Pre-mount dir fileno: {}", dfileno);

    qfs.mount("/mount", part);

    rf = qfs.resolve("/mount/testfile");
    rd = qfs.resolve("/mount/testdir");

    if (rf.node)
    {
        LogError("Pre-mount file preserved");
        return;
    }
    if (rd.node)
    {
        LogError("Pre-mount directory preserved");
        return;
    }

    qfs.touch("/mount/testfile_mnt");
    qfs.mkdir("/mount/testdir_mnt");

    rf = qfs.resolve("/mount/testfile_mnt");
    rd = qfs.resolve("/mount/testdir_mnt");

    if (!rf.node)
    {
        LogError("After-mount file not created");
        return;
    }
    if (!rd.node)
    {
        LogError("After-mount directory not created");
        return;
    }

    auto ffileno_mnt = rf.node->GetFileno();
    auto dfileno_mnt = rd.node->GetFileno();
    Log("New file in mounted directory fileno: {}", ffileno_mnt);
    Log("New directory in mounted directory fileno: {}", dfileno_mnt);

    if (ffileno == ffileno_mnt)
    {
        LogError("Pre-mount file is preserved after mount");
    }

    if (dfileno == dfileno_mnt)
    {
        LogError("Pre-mount file is preserved after mount");
    }

    qfs.unmount("/mount");

    rf = qfs.resolve("/mount/testfile");
    rd = qfs.resolve("/mount/testdir");

    auto ffileno_unmount = rf.node->GetFileno();
    auto dfileno_unmount = rd.node->GetFileno();
    Log("After unmount: /mount/testfile fileno: {}", ffileno);
    Log("After unmount: /mount/testdir fileno: {}", dfileno);

    if (ffileno != ffileno_unmount)
    {
        LogError("File changed after mount cycle. Before: {}, after: {}", ffileno, ffileno_unmount);
    }

    if (dfileno != dfileno_unmount)
    {
        LogError("Directory changed after mount cycle. Before: {}, after: {}", dfileno, dfileno_unmount);
    }

    if (qfs.resolve("/mount/testfile_mnt").node)
    {
        LogError("Mountpoint file persisted");
    }
    if (qfs.resolve("/mount/testdir_mnt").node)
    {
        LogError("Mountpoint directory persisted");
    }
}
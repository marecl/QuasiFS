#pragma once

#include "lib/include/quasi_fs.h"
#include "log.h"

using namespace QuasiFS;

void _printTree(const inode_ptr &node, const std::string &name, int depth)
{

    std::string depEnt = "\t";
    for (uint8_t q = 0; q < depth; q++)
    {
        depEnt = depEnt + "|--";
    }
    if (depth > 0)
        depEnt[depEnt.length() - 1] = '>';

    std::string type = "UNK";
    if (node->is_dir())
        type = "DIR";
    if (node->is_file())
        type = "FIL";
    if (node->is_link())
        type = "LNK";
    if (node->is_char())
        type = "CHR";

    if (!name.empty())
        std::cout << std::format("[{:3s}]\t", type) << std::format("[{:3d}]\t", node->fileno) << std::format("[{:3d}]\t", node->st.nlink) << depEnt << name << std::endl;
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
            std::cout << "\t\t\t" << depEnt << "[MOUNTPOINT]" << std::endl;
            _printTree(dir->mounted_root, "", depth + 1);
        }
        else
        {
            for (auto &[childName, child] : dir->entries)
            {
                _printTree(child, childName, depth + 1);
            }
        }
    }
}

void printTree(const inode_ptr &node, const std::string &name, int depth = 0)
{
    std::cout << "Type\tFileno\tnlink\ttree" << std::endl;
    _printTree(node, name, depth);
}

void TestTouchUnlinkFile(QFS &qfs);
void TestMkRmdir(QFS &qfs);
void TestMount(QFS &qfs);
void TestMountFileRetention(QFS &qfs);
void TestStLink(QFS &qfs)
{
    LogTest("File link counter");

    qfs.touch("/file");

    Resolved r{};
    int status = qfs.resolve("/file", r);

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

    Resolved r{};
    int status = qfs.resolve("/testfile", r);

    if (r.parent != qfs.GetRoot())
    {
        LogError("Wrong parent");
    }

    if (!r.node)
    {
        LogError("Not created");
    }

    status = qfs.unlink("/testfile");
    if (0 != status)
    {
        LogError("unlink failed");
    }

    status = qfs.resolve("/testfile", r);

    if (status != -ENOENT)
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

    Resolved r{};
    int status = qfs.resolve("/testdir", r);

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

    status = qfs.resolve("/testdir", r);

    if (-ENOENT != status)
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

    Resolved r{};
    int status = qfs.resolve("/dummy/mount", r);

    if (nullptr == r.node)
    {
        LogError("Pre-mount dir not found");
        return;
    }

    auto parent_from_root = r.parent->lookup("mount")->GetFileno();
    auto self_fileno = r.node->lookup(".")->GetFileno();
    auto parent_fileno = r.node->lookup("..")->GetFileno();

    Log("Pre-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root, parent_fileno, self_fileno);

    if (0 != qfs.mount("/dummy/mount", part))
    {
        LogError("Can't mount");
    }

    status = qfs.resolve("/dummy/mount", r);

    if (nullptr == r.node)
    {
        LogError("Post-mount dir not found");
        return;
    }

    if (r.mountpoint != part)
    {
        LogError("Bad partition");
    }

    status = qfs.resolve("/dummy", r);
    auto parent_from_root_mount = r.node->lookup("mount")->GetFileno();

    status = qfs.resolve("/dummy/mount", r);
    auto self_fileno_mount = r.node->lookup(".")->GetFileno();
    auto parent_fileno_mount = r.node->lookup("..")->GetFileno();

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

    if (r.node != part->GetRoot())
    {
        LogError("Bad node");
    }

    status = qfs.unmount("/dummy/mount");
    if (0 != status)
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

    Resolved rfile{};
    Resolved rdir{};

    int status_file = qfs.resolve("/mount/testfile", rfile);
    int status_dir = qfs.resolve("/mount/testdir", rdir);

    if (-ENOENT == status_file)
    {
        LogError("Test file not created");
        return;
    }
    if (-ENOENT == status_dir)
    {
        LogError("Test directory not created");
        return;
    }

    auto ffileno = rfile.node->GetFileno();
    auto dfileno = rdir.node->GetFileno();
    Log("Pre-mount file fileno: {}", ffileno);
    Log("Pre-mount dir fileno: {}", dfileno);

    qfs.mount("/mount", part);

    status_file = qfs.resolve("/mount/testfile", rfile);
    status_dir = qfs.resolve("/mount/testdir", rdir);

    if (-ENOENT != status_file)
    {
        LogError("Pre-mount file preserved");
        return;
    }
    if (-ENOENT != status_dir)
    {
        LogError("Pre-mount directory preserved");
        return;
    }

    qfs.touch("/mount/testfile_mnt");
    qfs.mkdir("/mount/testdir_mnt");

    status_file = qfs.resolve("/mount/testfile_mnt", rfile);
    status_dir = qfs.resolve("/mount/testdir_mnt", rdir);

    if (-ENOENT == status_file)
    {
        LogError("After-mount file not created");
        return;
    }
    if (-ENOENT == status_dir)
    {
        LogError("After-mount directory not created");
        return;
    }

    auto ffileno_mnt = rfile.node->GetFileno();
    auto dfileno_mnt = rdir.node->GetFileno();
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

    status_file = qfs.resolve("/mount/testfile", rfile);
    status_dir = qfs.resolve("/mount/testdir", rdir);

    auto ffileno_unmount = rfile.node->GetFileno();
    auto dfileno_unmount = rdir.node->GetFileno();
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

    status_file = qfs.resolve("/mount/testfile_mnt", rfile);
    status_dir = qfs.resolve("/mount/testdir_mnt", rdir);
    if (-ENOENT != status_file)
    {
        LogError("Mountpoint file persisted");
    }
    if (-ENOENT != status_dir)
    {
        LogError("Mountpoint directory persisted");
    }
}
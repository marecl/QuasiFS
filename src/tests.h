#pragma once

#include "quasifs/include/quasifs.h"
#include "log.h"
#include <sys/fcntl.h>

using namespace QuasiFS;

void _printTree(const inode_ptr &node, const std::string &name, int depth)
{

    std::string depEnt = "";
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
        std::cout << std::format("[{:3s}]\t", type) << std::format("[{:3d}]\t", node->fileno) << std::format("[{:3d}]\t", node->st.st_nlink) << depEnt << name << std::endl;
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

// Inode manip
void TestTouchUnlinkFile(QFS &qfs);
void TestMkRmdir(QFS &qfs);

// Mounts (partitions)
void TestMount(QFS &qfs);
void TestMountFileRetention(QFS &qfs);

// Links
void TestStLinkFile(QFS &qfs);
void TestStLinkDir(QFS &qfs) { LogTest("Unimplemented"); };
void TestStLinkMixed(QFS &qfs) { LogTest("Unimplemented"); };

// Symlinks
// TODO

// Files (I/O)
void TestFileOpen(QFS &qfs);
void TestFileops(QFS &qfs)
{
    LogTest("File R/W");

    if (int status = qfs.open("/fileop_open", 0); -ENOENT != status)
        LogError("Unexpected return on opening nonexistent file: {}", status);

    //
    qfs.touch("/fileop_open");
    //

    // int fd = qfs.open("/fileop_open", O_RDWR);
    // if (fd < 0)
    // {
    //     LogError("open() returned error: {}", fd);
    //     return;
    // }

    // const uint8_t bufW[] = "XDXDXDXD\nTest\00\11AAA\n";
    // const auto bufWsize = sizeof(bufW) / sizeof(uint8_t);
    // uint8_t bufR[128]{};

    // Log("Zapisano: {}", qfs.write(fd, bufW, bufWsize));
    // Log("Odczytano: {}", qfs.read(fd, bufR, 128));
    // qfs.close(fd);
}

// Directories (I/O)
void TestDirOpen(QFS &qfs);
void TestDirops(QFS &qfs) { LogTest("Unimplemented"); };

void Test(QFS &qfs)
{
    Log("");
    Log("QuasiFS Test Suite");
    Log("");

    // Inode manip
    TestTouchUnlinkFile(qfs);
    TestMkRmdir(qfs);

    // Mounts (partitions)
    TestMount(qfs);
    TestMountFileRetention(qfs);

    // Links
    TestStLinkFile(qfs);
    TestStLinkDir(qfs);
    TestStLinkMixed(qfs);

    // Symlinks
    // TODO

    // Files (I/O)
    TestFileOpen(qfs);
    TestFileops(qfs);

    // Directories (I/O)
    TestDirOpen(qfs);
    TestDirops(qfs);

    Log("");
    Log("Tests complete");
    Log("");
}

void TestTouchUnlinkFile(QFS &qfs)
{
    LogTest("Create a file");
    const fs::path path = "/testfile";
    Resolved r{};

    if (int status = qfs.touch(path); status != 0)
        LogError("touch {} : {}", path.string(), status);
    else
        LogSuccess("Touched {}", path.string());

    if (int status = qfs.resolve(path, r); status != 0)
        LogError("Can't resolve: {}", status);
    else
        LogSuccess("Resolved");

    if (r.parent != qfs.GetRoot())
        LogError("Wrong parent");

    if (int status = qfs.unlink(path); status != 0)
        LogError("unlink failed: {}", status);
    else
        LogSuccess("Unlinked");

    if (int status = qfs.resolve(path, r); status != -ENOENT)
        LogError("file not removed: {}", status);
    else
        LogSuccess("Removal confirmed");
}

void TestMkRmdir(QFS &qfs)
{
    LogTest("Create a directory");
    fs::path path = "/testdir";
    Resolved r{};

    if (int status = qfs.mkdir(path); status != 0)
        LogError("mkdir {} : {}", path.string(), status);
    else
        LogSuccess("mkdir'd {}", path.string());

    if (int status = qfs.resolve(path, r); status != 0)
        LogError("Can't resolve: {}", status);
    else
        LogSuccess("Resolved");

    if (r.parent != qfs.GetRoot())
        LogError("Wrong parent");

    if (int status = qfs.rmdir(path); status != 0)
        LogError("rmdir failed");
    else
        LogSuccess("rmdir'd");

    if (int status = qfs.resolve(path, r); -ENOENT != status)
        LogError("dir not removed: {}", status);
    else
        LogSuccess("Removal confirmed");
}

void TestMount(QFS &qfs)
{
    LogTest("Mount");

    auto part = Partition::Create();

    if (nullptr == part)
    {
        LogError("Can't create partition object");
        return;
    }

    Resolved r{};

    qfs.mkdir("/dummy");
    qfs.mkdir("/dummy/mount");

    if (int status = qfs.resolve("/dummy/mount", r); status != 0)
    {
        LogError("Mountpoint dir not created: {}", status);
        return;
    }
    else
        LogSuccess("mkdir'd /dummy/mount");

    if (nullptr == r.parent)
        LogError("Parent doesn't exist");

    if (nullptr == r.node)
        LogError("Child doesn't exist");

    auto parent_from_root = r.parent->lookup("mount")->GetFileno();
    auto self_fileno = std::static_pointer_cast<Directory>(r.node)->lookup(".")->GetFileno();
    auto parent_fileno = std::static_pointer_cast<Directory>(r.node)->lookup("..")->GetFileno();

    Log("\tPre-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root, parent_fileno, self_fileno);

    if (int status = qfs.mount("/dummy/mount", part); status != 0)
    {
        LogError("Can't mount: {}", status);
    }
    else
        LogSuccess("Mounted");

    if (int status = qfs.resolve("/dummy/mount", r); status != 0)
    {
        LogError("Can't resolve /dummy/mount after mounting: {}", status);
    }
    else
        LogSuccess("After-mount resolved");

    if (nullptr == r.node)
    {
        LogError("Post-mount dir not found");
        return;
    }

    if (r.mountpoint != part)
    {
        LogError("Bad partition");
    }

    qfs.resolve("/dummy", r);
    auto parent_from_root_mount = std::static_pointer_cast<Directory>(r.node)->lookup("mount")->GetFileno();

    qfs.resolve("/dummy/mount", r);
    auto self_fileno_mount = std::static_pointer_cast<Directory>(r.node)->lookup(".")->GetFileno();
    auto parent_fileno_mount = std::static_pointer_cast<Directory>(r.node)->lookup("..")->GetFileno();

    Log("\tPost-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root_mount, parent_fileno_mount, self_fileno_mount);
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
        LogError("Mountpoint root isn't partition root");
    }

    if (int status = qfs.unmount("/dummy/mount"); 0 != status)
    {
        LogError("Can't unmount: {}", status);
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

    if (int status_file = qfs.resolve("/mount/testfile", rfile); -ENOENT == status_file)
    {
        LogError("Test file not created: {}", status_file);
        return;
    }
    else
        LogSuccess("Test file created");
    if (int status_dir = qfs.resolve("/mount/testdir", rdir); -ENOENT == status_dir)
    {
        LogError("Test directory not created: {}", status_dir);
        return;
    }
    else
        LogSuccess("Test directory created");

    auto ffileno = rfile.node->GetFileno();
    auto dfileno = rdir.node->GetFileno();
    Log("\tPre-mount file fileno: {}", ffileno);
    Log("\tPre-mount dir fileno: {}", dfileno);

    qfs.mount("/mount", part);

    if (int status_file = qfs.resolve("/mount/testfile", rfile); -ENOENT != status_file)
    {
        LogError("Pre-mount file preserved (if 0), or error (if not ENOENT): {}", status_file);
        return;
    }
    if (int status_dir = qfs.resolve("/mount/testdir", rdir); -ENOENT != status_dir)
    {
        LogError("Pre-mount directory preserved (if 0), or error (if not ENOENT): {}", status_dir);
        return;
    }

    qfs.touch("/mount/testfile_mnt");
    qfs.mkdir("/mount/testdir_mnt");

    if (int status_file = qfs.resolve("/mount/testfile_mnt", rfile); -ENOENT == status_file)
    {
        LogError("After-mount file not created: {}", status_file);
        return;
    }
    else
        LogSuccess("Test file created (mount)");
    if (int status_dir = qfs.resolve("/mount/testdir_mnt", rdir); -ENOENT == status_dir)
    {
        LogError("After-mount directory not created: {}", status_dir);
        return;
    }
    else
        LogSuccess("Test dir created (mount)");

    auto ffileno_mnt = rfile.node->GetFileno();
    auto dfileno_mnt = rdir.node->GetFileno();
    Log("\tNew file in mounted directory fileno: {}", ffileno_mnt);
    Log("\tNew directory in mounted directory fileno: {}", dfileno_mnt);

    if (ffileno == ffileno_mnt)
    {
        LogError("Pre-mount file is preserved after mount");
    }

    if (dfileno == dfileno_mnt)
    {
        LogError("Pre-mount file is preserved after mount");
    }

    qfs.unmount("/mount");

    qfs.resolve("/mount/testfile", rfile);
    qfs.resolve("/mount/testdir", rdir);
    auto ffileno_unmount = rfile.node->GetFileno();
    auto dfileno_unmount = rdir.node->GetFileno();
    Log("\tAfter unmount: /mount/testfile fileno: {}", ffileno);
    Log("\tAfter unmount: /mount/testdir fileno: {}", dfileno);

    if (ffileno != ffileno_unmount)
    {
        LogError("File changed after mount cycle. Before: {}, after: {}", ffileno, ffileno_unmount);
    }

    if (dfileno != dfileno_unmount)
    {
        LogError("Directory changed after mount cycle. Before: {}, after: {}", dfileno, dfileno_unmount);
    }

    if (int status_file = qfs.resolve("/mount/testfile_mnt", rfile); -ENOENT != status_file)
    {
        LogError("Mountpoint file persisted");
    }
    else
        LogSuccess("Mountpoint file doesn't exist");
    if (int status_dir = qfs.resolve("/mount/testdir_mnt", rdir); -ENOENT != status_dir)
    {
        LogError("Mountpoint directory persisted");
    }
    else
        LogSuccess("Mountpoint dir doesn't exist");
}

void TestStLinkFile(QFS &qfs)
{
    LogTest("File link counter");

    Resolved r{};

    qfs.touch("/file");
    qfs.resolve("/file", r);

    file_ptr f = std::reinterpret_pointer_cast<RegularFile>(r.node);

    auto nlink = &f->st.st_nlink;

    if (*nlink != 1)
    {
        LogError("New file link {} != 1", *nlink);
    }
    else
        LogSuccess("Created new file. nlink=1", *nlink);

    if (int status = qfs.link("/file", "/file2"); 0 != status)
    {
        LogError("Link existing file failed");
    }
    else
        LogSuccess("File linked");

    if (*nlink != 2)
    {
        LogError("New file link {} != 2", *nlink);
    }
    else
        LogSuccess("nlink=2");

    if (int status = qfs.unlink("/file"); 0 != status)
    {
        LogError("Unlink OG file failed: {}", status);
    }
    else
        LogSuccess("File unlinked");

    if (*nlink != 1)
    {
        LogError("New file link {} != 1", *nlink);
    }
    else
        LogSuccess("nlink=1");

    if (int status = qfs.unlink("/file2"); 0 != status)
    {
        LogError("Unlink new-linked file failed: {}", status);
    }
    else
        LogSuccess("File unlinked");

    if (*nlink != 0)
    {
        LogError("New file link {} != 0", *nlink);
    }
    else
        LogSuccess("nlink=1");

    inode_ptr readback = qfs.GetRootFS()->GetInode(f->GetFileno());

    if (nullptr != readback)
    {
        LogError("inode wasn't erased from partition after removing all links");
    }
}

void TestFileOpen(QFS &qfs)
{
    LogTest("File open/close");

    Resolved r{};

    if (int status = qfs.open("/file_open", 0); -ENOENT == status)
    {
        LogSuccess("nonexistent file, O_READ");
        qfs.close(status);
    }
    else
        LogError("O_READ, nonexistent file: {}", status);

    if (int status = qfs.open("/file_open", O_CREAT); 0 == status)
    {
        LogSuccess("nonexistent file, O_READ | O_CREAT");
        qfs.close(status);
    }
    else
        LogError("O_READ | O_CREAT, nonexistent file: {}", status);

    //
    if (-ENOENT == qfs.resolve("/file_open", r))
        LogError("O_CREAT didn't create the file");
    //

    if (int status = qfs.open("/file_open", O_RDWR); 0 == status)
    {
        LogSuccess("existing file, O_RDWR");
        qfs.close(status);
    }
    else
        LogError("O_RDWR, existing file: {}", status);

    // Edge case - undefined in POSIX
    // We can open the file for reading only and truncate it anyway
    if (int status = qfs.open("/file_open", O_RDONLY | O_TRUNC); 0 == status)
    {
        LogSuccess("existing file, O_RDONLY | O_TRUNC");
        qfs.close(status);
    }
    else
        LogError("O_RDONLY | O_TRUNC, existing file: {}", status);
}

void TestDirOpen(QFS &qfs)
{
    LogTest("Dir open/close");

    Resolved r{};

    if (int status = qfs.open("/dir_open", O_DIRECTORY); -ENOENT == status)
    {
        LogSuccess("nonexistent dir, O_READ | O_DIRECTORY");
        qfs.close(status);
    }
    else
        LogError("nonexistent dir, O_READ | O_DIRECTORY: {}", status);

    if (int status = qfs.open("/dir_open", O_DIRECTORY | O_WRONLY); -ENOENT == status)
    {
        LogSuccess("nonexistent dir, O_WRONLY | O_DIRECTORY");
        qfs.close(status);
    }
    else
        LogError("nonexistent dir, O_WRONLY | O_DIRECTORY: {}", status);

    //
    qfs.mkdir("/dir_open");
    //

    if (int status = qfs.open("/dir_open", O_DIRECTORY); 0 == status)
    {
        LogSuccess("existing dir, O_RDONLY | O_DIRECTORY");
        qfs.close(status);
    }
    else
        LogError("existing dir, O_RDONLY | O_DIRECTORY: {}", status);

    if (int status = qfs.open("/dir_open", 0); 0 == status)
    {
        LogSuccess("existing dir, O_RDONLY");
        qfs.close(status);
    }
    else
        LogError("existing dir, O_RDONLY: {}", status);

    //
    qfs.touch("/notadir");
    //

    if (int status = qfs.open("/notadir", O_DIRECTORY); -ENOTDIR == status)
    {
        LogSuccess("not a dir, O_DIRECTORY");
        qfs.close(status);
    }
    else
        LogError("not a dir, O_DIRECTORY: {}", status);
}
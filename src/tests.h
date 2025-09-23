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

#define UNIMPLEMENTED()                                       \
    {                                                         \
        LogTest("Unimplemented ({}:{})", __FILE__, __LINE__); \
    }

// Path resolution
void TestResolve(QFS &qfs) UNIMPLEMENTED();

// Inode manip
void TestTouchUnlinkFile(QFS &qfs);
void TestMkRmdir(QFS &qfs);

// Mounts (partitions)
void TestMount(QFS &qfs);
void TestMountFileRetention(QFS &qfs);

// Links
void TestStLinkFile(QFS &qfs);
void TestStLinkDir(QFS &qfs);
void TestStLinkMixed(QFS &qfs) UNIMPLEMENTED();

// Symlinks
void TestSymlinkFile(QFS &qfs) UNIMPLEMENTED();
void TestSymlinkDir(QFS &qfs) UNIMPLEMENTED();

// Files (I/O)
void TestFileOpen(QFS &qfs);
void TestFileops(QFS &qfs) UNIMPLEMENTED();

// Directories (I/O)
void TestDirOpen(QFS &qfs);
void TestDirops(QFS &qfs) UNIMPLEMENTED();

void Test(QFS &qfs)
{
    Log("");
    Log("QuasiFS Test Suite");
    Log("");

    // Path resolution
    TestResolve(qfs);

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
    TestSymlinkFile(qfs);
    TestSymlinkDir(qfs);

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

    if (int status = qfs.Touch(path); status == 0)
        LogSuccess("Touched {}", path.string());
    else
        LogError("touch {} : {}", path.string(), status);

    if (int status = qfs.Resolve(path, r); status == 0)
        LogSuccess("Resolved");
    else
        LogError("Can't resolve: {}", status);

    if (r.parent != qfs.GetRoot())
        LogError("Wrong parent");

    if (int status = qfs.Unlink(path); status == 0)
        LogSuccess("Unlinked");
    else
        LogError("unlink failed: {}", status);

    if (int status = qfs.Resolve(path, r); status == -QUASI_ENOENT)
        LogSuccess("Removal confirmed");
    else
        LogError("file not removed: {}", status);
}

void TestMkRmdir(QFS &qfs)
{
    LogTest("Create a directory");
    fs::path path = "/testdir";
    Resolved r{};

    if (int status = qfs.MKDir(path); status == 0)
        LogSuccess("mkdir'd {}", path.string());
    else
        LogError("mkdir {} : {}", path.string(), status);

    if (int status = qfs.Resolve(path, r); status == 0)
        LogSuccess("Resolved");
    else
        LogError("Can't resolve: {}", status);

    if (r.parent != qfs.GetRoot())
        LogError("Wrong parent");

    if (int status = qfs.RMDir(path); status == 0)
        LogSuccess("rmdir'd");
    else
        LogError("rmdir failed");

    if (int status = qfs.Resolve(path, r); -QUASI_ENOENT == status)
        LogSuccess("Removal confirmed");
    else
        LogError("dir not removed: {}", status);

    qfs.MKDir("/dir1");
    path = "/dir1/dir2";

    if (int status = qfs.MKDir(path); status == 0)
        LogSuccess("mkdir'd {}", path.string());
    else
        LogError("mkdir {} : {}", path.string(), status);

    if (int status = qfs.Resolve(path, r); status == 0)
        LogSuccess("Resolved");
    else
        LogError("Can't resolve: {}", status);

    if (int status = qfs.RMDir(path); status == 0)
        LogSuccess("rmdir'd");
    else
        LogError("rmdir failed");

    if (int status = qfs.Resolve(path, r); -QUASI_ENOENT == status)
        LogSuccess("Removal confirmed");
    else
        LogError("dir not removed: {}", status);
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

    qfs.MKDir("/dummy");
    qfs.MKDir("/dummy/mount");

    if (int status = qfs.Resolve("/dummy/mount", r); status == 0)
        LogSuccess("mkdir'd /dummy/mount");
    else
    {
        LogError("Mountpoint dir not created: {}", status);
        return;
    }

    if (nullptr == r.parent)
        LogError("Parent doesn't exist");

    if (nullptr == r.node)
        LogError("Child doesn't exist");

    auto parent_from_root = r.parent->lookup("mount")->GetFileno();
    auto self_fileno = std::static_pointer_cast<Directory>(r.node)->lookup(".")->GetFileno();
    auto parent_fileno = std::static_pointer_cast<Directory>(r.node)->lookup("..")->GetFileno();

    Log("\tPre-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root, parent_fileno, self_fileno);

    if (int status = qfs.Mount("/dummy/mount", part); status == 0)
        LogSuccess("Mounted");
    else
        LogError("Can't mount: {}", status);

    if (int status = qfs.Resolve("/dummy/mount", r); status == 0)
        LogSuccess("After-mount resolved");
    else
        LogError("Can't resolve /dummy/mount after mounting: {}", status);

    if (nullptr == r.node)
    {
        LogError("Post-mount dir not found");
        return;
    }

    if (r.mountpoint != part)
    {
        LogError("Bad partition");
    }

    qfs.Resolve("/dummy", r);
    auto parent_from_root_mount = std::static_pointer_cast<Directory>(r.node)->lookup("mount")->GetFileno();

    qfs.Resolve("/dummy/mount", r);
    auto self_fileno_mount = std::static_pointer_cast<Directory>(r.node)->lookup(".")->GetFileno();
    auto parent_fileno_mount = std::static_pointer_cast<Directory>(r.node)->lookup("..")->GetFileno();

    Log("\tPost-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root_mount, parent_fileno_mount, self_fileno_mount);
    if (2 != self_fileno_mount)
    {
        LogError("Mountpoint root fileno isn't 2");
    }

    if (2 != parent_fileno_mount)
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

    if (int status = qfs.Unmount("/dummy/mount"); 0 == status)
        LogSuccess("Unmounted");
    else
        LogError("Can't unmount: {}", status);

    qfs.RMDir("/dummy/mount");
    qfs.RMDir("/dummy");
}

void TestMountFileRetention(QFS &qfs)
{
    LogTest("File retention/substitution on remount");

    auto part = Partition::Create();

    qfs.MKDir("/mount");

    qfs.Touch("/mount/dummy1"); // dummy files to shift fileno
    qfs.Touch("/mount/dummy2");
    qfs.Touch("/mount/dummy3");
    qfs.Touch("/mount/dummy4");
    qfs.Touch("/mount/testfile");
    qfs.MKDir("/mount/testdir");

    Resolved rfile{};
    Resolved rdir{};

    if (int status_file = qfs.Resolve("/mount/testfile", rfile); 0 == status_file)
        LogSuccess("Test file created");
    else
    {
        LogError("Test file not created: {}", status_file);
        return;
    }

    if (int status_dir = qfs.Resolve("/mount/testdir", rdir); 0 == status_dir)
        LogSuccess("Test directory created");
    else
    {
        LogError("Test directory not created: {}", status_dir);
        return;
    }

    auto ffileno = rfile.node->GetFileno();
    auto dfileno = rdir.node->GetFileno();
    Log("\tPre-mount file fileno: {}", ffileno);
    Log("\tPre-mount dir fileno: {}", dfileno);

    qfs.Mount("/mount", part);

    if (int status_file = qfs.Resolve("/mount/testfile", rfile); 0 == status_file && -QUASI_ENOENT == status_file)
    {
        LogError("Pre-mount file preserved (if 0), or error (if not ENOENT): {}", status_file);
        return;
    }
    if (int status_dir = qfs.Resolve("/mount/testdir", rdir); 0 == status_dir && -QUASI_ENOENT == status_dir)
    {
        LogError("Pre-mount directory preserved (if 0), or error (if not ENOENT): {}", status_dir);
        return;
    }

    qfs.Touch("/mount/testfile_mnt");
    qfs.MKDir("/mount/testdir_mnt");

    if (int status_file = qfs.Resolve("/mount/testfile_mnt", rfile); -QUASI_ENOENT != status_file)
        LogSuccess("Test file created (mount)");
    else
    {
        LogError("After-mount file not created: {}", status_file);
        return;
    }

    if (int status_dir = qfs.Resolve("/mount/testdir_mnt", rdir); -QUASI_ENOENT != status_dir)
        LogSuccess("Test dir created (mount)");
    else
    {
        LogError("After-mount directory not created: {}", status_dir);
        return;
    }

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

    qfs.Unmount("/mount");

    qfs.Resolve("/mount/testfile", rfile);
    qfs.Resolve("/mount/testdir", rdir);
    auto ffileno_unmount = rfile.node->GetFileno();
    auto dfileno_unmount = rdir.node->GetFileno();
    Log("\tAfter unmount: /mount/testfile fileno: {}", ffileno);
    Log("\tAfter unmount: /mount/testdir fileno: {}", dfileno);

    if (ffileno != ffileno_unmount)
        LogError("File changed after mount cycle. Before: {}, after: {}", ffileno, ffileno_unmount);

    if (dfileno != dfileno_unmount)
        LogError("Directory changed after mount cycle. Before: {}, after: {}", dfileno, dfileno_unmount);

    if (int status_file = qfs.Resolve("/mount/testfile_mnt", rfile); -QUASI_ENOENT == status_file)
        LogSuccess("Mountpoint file doesn't exist");
    else
        LogError("Mountpoint file persisted");

    if (int status_dir = qfs.Resolve("/mount/testdir_mnt", rdir); -QUASI_ENOENT == status_dir)
        LogSuccess("Mountpoint dir doesn't exist");
    else
        LogError("Mountpoint directory persisted");
}

void TestStLinkFile(QFS &qfs)
{
    LogTest("File link counter");

    Resolved r{};

    qfs.Touch("/file");
    qfs.Resolve("/file", r);

    file_ptr f = std::reinterpret_pointer_cast<RegularFile>(r.node);

    auto nlink = &f->st.st_nlink;

    if (1 == *nlink)
        LogSuccess("Created new file. nlink=1", *nlink);
    else
        LogError("New file link {} != 1", *nlink);

    if (int status = qfs.Link("/file", "/file2"); 0 == status)
        LogSuccess("File linked");
    else
        LogError("Link existing file failed");

    if (2 == *nlink)
        LogSuccess("nlink ==2");
    else
        LogError("New file link {} != 2", *nlink);

    if (int status = qfs.Unlink("/file"); 0 == status)
        LogSuccess("File unlinked");
    else
        LogError("Unlink OG file failed: {}", status);

    if (1 == *nlink)
        LogSuccess("nlink ==1");
    else
        LogError("New file link {} != 1", *nlink);

    if (int status = qfs.Unlink("/file2"); 0 == status)
        LogSuccess("File unlinked");
    else
        LogError("Unlink new-linked file failed: {}", status);

    if (*nlink == 0)
        LogSuccess("nlink ==0");
    else
        LogError("New file link {} != 0", *nlink);

    inode_ptr readback = qfs.GetRootFS()->GetInodeByFileno(f->GetFileno());

    if (nullptr == readback)
        LogSuccess("File removed from inode table after removing all links");
    else
        LogError("File not removed from inode table after removing all links");
}

void TestStLinkDir(QFS &qfs)
{
    LogTest("Dir link counter");

    Resolved r{};

    qfs.MKDir("/dir");
    qfs.Resolve("/dir", r);

    dir_ptr f = std::reinterpret_pointer_cast<Directory>(r.node);

    auto nlink = &f->st.st_nlink;

    if (2 == *nlink)
        LogSuccess("Created new file. nlink=2", *nlink);
    else
        LogError("New dir link {} != 2", *nlink);

    if (int status = qfs.Link("/dir", "/dir2"); -QUASI_EPERM == status)
        LogSuccess("Dir can't be linked");
    else
        LogError("Dir linked: {}", status);

    if (int status = qfs.MKDir("/dir/dir2"); 0 == status)
        LogSuccess("Subdir created");
    else
        LogError("Can't create subdir");

    if (3 == *nlink)
        LogSuccess("Dir with subdir nlink==3");
    else
        LogError("Dir with subdir nlink {} != 3", *nlink);

    if (int status = qfs.RMDir("/dir/dir2"); 0 == status)
        LogSuccess("subdir unlinked");
    else
        LogError("Unlink subdir failed: {}", status);

    if (2 == *nlink)
        LogSuccess("Dir nlink decreased after removing subdir nlink==2");
    else
        LogError("Dir nlink didn't decrease after removing subdir nlink: {} != 2", *nlink);

    if (int status = qfs.RMDir("/dir"); 0 == status)
        LogSuccess("subdir unlinked");
    else
        LogError("Unlink subdir failed: {}", status);

    if (*nlink == 0)
        LogSuccess("Dir nlink==0", *nlink);
    else
        LogError("Dir nlink didn't decrease after removing itself: {} != 0", *nlink);

    inode_ptr readback = qfs.GetRootFS()->GetInodeByFileno(f->GetFileno());

    if (nullptr == readback)
        LogSuccess("Dir removed from inode table after removing all links");
    else
        LogError("Dir not removed from inode table after removing all links");
};

void TestFileOpen(QFS &qfs)
{
    LogTest("File open/close");

    Resolved r{};

    if (int status = qfs.Open("/file_open", 0, 0); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent file, O_READ");
        qfs.Close(status);
    }
    else
        LogError("O_READ, nonexistent file: {}", status);

    if (int status = qfs.Open("/file_open", O_CREAT, 0); 0 == status)
    {
        LogSuccess("nonexistent file, O_READ | O_CREAT");
        qfs.Close(status);
    }
    else
        LogError("O_READ | O_CREAT, nonexistent file: {}", status);

    //
    if (-QUASI_ENOENT == qfs.Resolve("/file_open", r))
        LogError("O_CREAT didn't create the file");
    //

    if (int status = qfs.Open("/file_open", O_RDWR, 0); 0 == status)
    {
        LogSuccess("existing file, O_RDWR");
        qfs.Close(status);
    }
    else
        LogError("O_RDWR, existing file: {}", status);

    // Edge case - undefined in POSIX
    // We can open the file for reading only and truncate it anyway
    if (int status = qfs.Open("/file_open", O_RDONLY | O_TRUNC, 0); 0 == status)
    {
        LogSuccess("existing file, O_RDONLY | O_TRUNC");
        qfs.Close(status);
    }
    else
        LogError("O_RDONLY | O_TRUNC, existing file: {}", status);
}

void TestDirOpen(QFS &qfs)
{
    LogTest("Dir open/close");

    Resolved r{};

    if (int status = qfs.Open("/dir_open", O_DIRECTORY, 0); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent dir, O_READ | O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("nonexistent dir, O_READ | O_DIRECTORY: {}", status);

    if (int status = qfs.Open("/dir_open", O_DIRECTORY | O_WRONLY, 0); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent dir, O_WRONLY | O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("nonexistent dir, O_WRONLY | O_DIRECTORY: {}", status);

    //
    qfs.MKDir("/dir_open");
    //

    if (int status = qfs.Open("/dir_open", O_DIRECTORY, 0); 0 == status)
    {
        LogSuccess("existing dir, O_RDONLY | O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("existing dir, O_RDONLY | O_DIRECTORY: {}", status);

    if (int status = qfs.Open("/dir_open", 0, 0); 0 == status)
    {
        LogSuccess("existing dir, O_RDONLY");
        qfs.Close(status);
    }
    else
        LogError("existing dir, O_RDONLY: {}", status);

    //
    qfs.Touch("/notadir");
    //

    if (int status = qfs.Open("/notadir", O_DIRECTORY, 0); -QUASI_ENOTDIR == status)
    {
        LogSuccess("not a dir, O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("not a dir, O_DIRECTORY: {}", status);
}
#pragma once

#include "quasifs/include/quasifs_inode_directory.h"
#include "quasifs/include/quasifs_inode_regularfile.h"
#include "quasifs/include/quasifs_inode_symlink.h"
#include "quasifs/include/quasifs_partition.h"

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
    {
        std::cout << std::format("[{:3s}]\t", type) << std::format("[{:3d}]\t", node->fileno) << std::format("[{:3d}]\t", node->st.st_nlink) << depEnt << name;
        if (node->is_link())
            std::cout << "\t->\t" << std::static_pointer_cast<Symlink>(node)->follow();
        std::cout << std::endl;
    }
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

// Path resolution
void TestResolve(QFS &qfs);

// Inode manip
void TestTouchUnlinkFile(QFS &qfs);
void TestMkRmdir(QFS &qfs);

// Mounts (partitions)
void TestMount(QFS &qfs);
void TestMountFileRetention(QFS &qfs);
void TestMountRO(QFS &qfs)
{
    partition_ptr ropart = Partition::Create();

    qfs.MKDir("/ro");
    if (int mount_status = qfs.Mount("/ro", ropart, MountOptions::MOUNT_NOOPT); mount_status == 0)
        LogSuccess("Mounted /ro as read-only partition");
    else
        LogError("Can't mount /ro : {}", mount_status);

    if (int fd = qfs.Creat("/ro/bad"); fd == -QUASI_EROFS)
        LogSuccess("Can't create a file in RO partition");
    else
    {
        LogError("Unexpected return from creat(): {}", fd);
        Resolved r;
        if (int resolve_status = qfs.Resolve("/ro/bad", r); resolve_status == 0)
            LogError("Created a file in RO partition");
    }
}

// Links
void TestStLinkFile(QFS &qfs);
void TestStLinkDir(QFS &qfs);
void TestStLinkMixed(QFS &qfs);

// Symlinks
void TestSymlinkFile(QFS &qfs);
void TestSymlinkDir(QFS &qfs);
void TestSymlinkCursed(QFS &qfs);

// Files (I/O)
void TestFileOpen(QFS &qfs);
void TestFileOps(QFS &qfs);
void TestFileSeek(QFS &qfs);

// Directories (I/O)
void TestDirOpen(QFS &qfs);
void TestDirOps(QFS &qfs);

// Stat
void TestStat(QFS &qfs)
{
    qfs.MKDir("/mount");

    partition_ptr part = Partition::Create("", MountOptions::MOUNT_RW);
    qfs.Mount("/mount", part);

    qfs.Creat("/mount/file");
}
void TestHostStat(QFS &qfs) UNIMPLEMENTED();

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
    TestMountRO(qfs);

    // Links
    TestStLinkFile(qfs);
    TestStLinkDir(qfs);
    TestStLinkMixed(qfs);

    // Symlinks
    TestSymlinkFile(qfs);
    TestSymlinkDir(qfs);
    TestSymlinkCursed(qfs);

    // Files (I/O)
    TestFileOpen(qfs);
    TestFileOps(qfs);
    TestFileSeek(qfs);

    // Directories (I/O)
    TestDirOpen(qfs);
    TestDirOps(qfs);

    // Stat
    TestStat(qfs);
    TestHostStat(qfs);

    Log("");
    Log("Tests complete");
    Log("");
}

//
// Path resolution
//

void TestResolve(QFS &qfs) { UNIMPLEMENTED() }

//
// Inode manip
//

void TestTouchUnlinkFile(QFS &qfs)
{
    LogTest("Create a file");
    const fs::path path = "/testfile";
    Resolved r{};

    if (int status = qfs.Creat(path); status >= 0)
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

//
// Mounts (partitions)
//

void TestMount(QFS &qfs)
{
    LogTest("Mount");

    partition_ptr part = Partition::Create();

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

    qfs.Creat("/mount/dummy1"); // dummy files to shift fileno
    qfs.Creat("/mount/dummy2");
    qfs.Creat("/mount/dummy3");
    qfs.Creat("/mount/dummy4");
    qfs.Creat("/mount/testfile");
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

    if (int status = qfs.Mount("/mount", part); 0 == status)
        LogSuccess("Mounted /mount");
    else
        LogError("Can't mount /mount : {}", status);

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

    qfs.Creat("/mount/testfile_mnt");
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

    if (int status = qfs.Unmount("/mount"); 0 == status)
        LogSuccess("Unmounted");
    else
        LogError("Can't unmount: {}", status);

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

//
// Links
//

void TestStLinkFile(QFS &qfs)
{
    LogTest("File link counter");

    Resolved r{};

    qfs.Creat("/file");
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

void TestStLinkMixed(QFS &qfs) { UNIMPLEMENTED() }

//
// Symlinks
//

void TestSymlinkFile(QFS &qfs)
{
    LogTest("Symlinking files");

    qfs.Close(qfs.Creat("/link_source_file"));
    qfs.MKDir("/oneup");
    qfs.Close(qfs.Creat("/oneup/link_source_file_up"));

    Resolved r;

    int resolve_status = qfs.Resolve("/link_source_file", r);
    const file_ptr src_file = std::static_pointer_cast<RegularFile>(r.node);
    resolve_status = qfs.Resolve("/oneup/link_source_file_up", r);
    const file_ptr src_file_up = std::static_pointer_cast<RegularFile>(r.node);
    file_ptr dst_file;

    // link, same directory
    qfs.LinkSymbolic("/link_source_file", "/link_dest_file");
    resolve_status = qfs.Resolve("/link_dest_file", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);
    if (src_file == dst_file)
        LogSuccess("Same directory symlink works");
    else
        LogError("Same directory symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.LinkSymbolic("/oneup/link_source_file_up", "/oneup/link_dest_file_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_file_up", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);
    if (src_file_up == dst_file)
        LogSuccess("Same directory, 1 level deep symlink works");
    else
        LogError("Same directory 1 level deep symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.LinkSymbolic("/oneup/link_source_file_up", "/link_dest_file_to_down");
    resolve_status = qfs.Resolve("/link_dest_file_to_down", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);
    if (src_file_up == dst_file)
        LogSuccess("Same directory, up->down works");
    else
        LogError("Same directory up->down failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.LinkSymbolic("/link_source_file", "/oneup/link_dest_file_to_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_file_to_up", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);
    if (src_file == dst_file)
        LogSuccess("Same directory, down->up works");
    else
        LogError("Same directory down->up failed: Resolve returned {}", resolve_status);

    return;
}

void TestSymlinkDir(QFS &qfs)
{
    LogTest("Symlinking directories");

    qfs.MKDir("/link_source_dir");
    qfs.MKDir("/oneup");
    qfs.MKDir("/oneup/link_source_dir_up");

    Resolved r;

    int resolve_status = qfs.Resolve("/link_source_dir", r);
    const dir_ptr src_dir = std::static_pointer_cast<Directory>(r.node);
    resolve_status = qfs.Resolve("/oneup/link_source_dir_up", r);
    const dir_ptr src_dir_up = std::static_pointer_cast<Directory>(r.node);
    dir_ptr dst_dir;

    // link, same directory
    qfs.LinkSymbolic("/link_source_dir", "/link_dest_dir");
    resolve_status = qfs.Resolve("/link_dest_dir", r);
    dst_dir = std::static_pointer_cast<Directory>(r.node);
    if (src_dir == dst_dir)
        LogSuccess("Same directory symlink works");
    else
        LogError("Same directory symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.LinkSymbolic("/oneup/link_source_dir_up", "/oneup/link_dest_dir_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_dir_up", r);
    dst_dir = std::static_pointer_cast<Directory>(r.node);
    if (src_dir_up == dst_dir)
        LogSuccess("Same directory, 1 level deep symlink works");
    else
        LogError("Same directory 1 level deep symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.LinkSymbolic("/oneup/link_source_dir_up", "/link_dest_dir_to_down");
    resolve_status = qfs.Resolve("/link_dest_dir_to_down", r);
    dst_dir = std::static_pointer_cast<Directory>(r.node);
    if (src_dir_up == dst_dir)
        LogSuccess("Same directory, up->down works");
    else
        LogError("Same directory up->down failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.LinkSymbolic("/link_source_dir", "/oneup/link_dest_dir_to_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_dir_to_up", r);
    dst_dir = std::static_pointer_cast<Directory>(r.node);
    if (src_dir == dst_dir)
        LogSuccess("Same directory, down->up works");
    else
        LogError("Same directory down->up failed: Resolve returned {}", resolve_status);

    return;
}

void TestSymlinkCursed(QFS &qfs)
{
    LogTest("Cursed links");

    qfs.Creat("/src_file");
    qfs.MKDir("/layer1");
    qfs.MKDir("/layer1/layer2");
    qfs.MKDir("/layer1/layer2/layer3");
    qfs.MKDir("/layer1/layer2/layer3/layer4");

    Resolved r;
    int resolve_status = qfs.Resolve("/src_file", r);
    const file_ptr src_file = std::static_pointer_cast<RegularFile>(r.node);
    if (src_file == nullptr)
        LogError("Didn't create test file");
    file_ptr dst_file;

    // linking directories
    qfs.LinkSymbolic("/layer1/layer2/layer3/layer4", "/l4");
    qfs.LinkSymbolic("/src_file", "/l4/sos");
    resolve_status = qfs.Resolve("/l4/sos", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);

    if (src_file == dst_file)
        LogSuccess("File symlinked to symlinked directory tree");
    else
        LogError("File symlinked to symlinked directory tree: Resolve returned {}", resolve_status);

    // linking directories
    qfs.LinkSymbolic("/l4/sos", "/l4/sos2");
    resolve_status = qfs.Resolve("/l4/sos2", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);

    if (src_file == dst_file)
        LogSuccess("File symlinked to file symlinked in symlinked directory tree");
    else
        LogError("File symlinked to file symlinked in symlinked directory tree: Resolve returned {}", resolve_status);

    qfs.LinkSymbolic("/layer1/layer2/layer3/layer4", "/layer1/layer2/early4");
    qfs.LinkSymbolic("/src_file", "/layer1/layer2/early4/early_file");
    resolve_status = qfs.Resolve("/layer1/layer2/early4/early_file", r);
    dst_file = std::static_pointer_cast<RegularFile>(r.node);

    if (src_file == dst_file)
        LogSuccess("File symlinked to file symlinked in a directory symlinked within directory tree");
    else
        LogError("File symlinked to file symlinked in a directory symlinked within directory tree: Resolve returned {}", resolve_status);

    // loops
    qfs.MKDir("/tmp");
    qfs.LinkSymbolic("/tmp/loopLinkDir", "/tmp/loopLinkDir");
    resolve_status = qfs.Resolve("/tmp/loopLinkDir", r);
    if (-QUASI_ELOOP == resolve_status)
        LogSuccess("Symlink loop safeguard works for directories");
    else
        LogError("Symlink loop safeguard for directories didn't trigger: Resolve returned {}", resolve_status);

    qfs.LinkSymbolic("/tmp/loopLinkFile", "/tmp/loopLinkFile");
    resolve_status = qfs.Resolve("/tmp/loopLinkFile", r);
    if (-QUASI_ELOOP == resolve_status)
        LogSuccess("Symlink loop safeguard works for files");
    else
        LogError("Symlink loop safeguard for files didn't trigger: Resolve returned {}", resolve_status);

    qfs.Creat("/missing_file");
    qfs.LinkSymbolic("/missing_file", "/dangling_file");
    qfs.Unlink("/missing_file");
    resolve_status = qfs.Resolve("/dangling_file", r);

    if (-QUASI_ENOENT == resolve_status)
        LogSuccess("Dangling symlink returns ENOENT");
    else
        LogError("Resolving dangling symlink returns wrong error: ", resolve_status);

    qfs.MKDir("/dir1");
    qfs.MKDir("/dir2");
    qfs.Creat("/dir2/XD");
    qfs.LinkSymbolic("/dir2", "/dir1/missing");

    Log("These 3 tests will fail, because this feature is unimplemented yet");

    resolve_status = qfs.Resolve("/dir1/missing/XD", r);
    if (0 == resolve_status)
        LogSuccess("Resolved file in directory symlinked inside a tree");
    else
        LogError("Didn't resolve file in directory symlinked inside a tree: ", resolve_status);

    if (0 != qfs.Unlink("/dir1/missing"))
    {
        LogError("Unlink should remove symlink when path is not preceeded with /");
    }

    resolve_status = qfs.Resolve("/dir1/missing/XD", r);

    if (-QUASI_ENOENT == resolve_status)
        LogSuccess("Dangling symlink inside a tree returns ENOENT");
    else
        LogError("Resolving dangling symlink inside a tree returns wrong error: {}", resolve_status);
}

//
// Files (I/O)
//

void TestFileOpen(QFS &qfs)
{
    LogTest("File open/close");

    Resolved r{};

    if (int status = qfs.Open("/file_open", 0); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent file, O_READ");
        qfs.Close(status);
    }
    else
        LogError("O_READ, nonexistent file: {}", status);

    if (int status = qfs.Open("/file_open", O_CREAT); 0 <= status)
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

    if (int status = qfs.Open("/file_open", O_RDWR); 0 <= status)
    {
        LogSuccess("existing file, O_RDWR");
        qfs.Close(status);
    }
    else
        LogError("O_RDWR, existing file: {}", status);

    // Edge case - undefined in POSIX
    // We can open the file for reading only and truncate it anyway
    if (int status = qfs.Open("/file_open", O_RDONLY | O_TRUNC); 0 <= status)
    {
        LogSuccess("existing file, O_RDONLY | O_TRUNC");
        qfs.Close(status);
    }
    else
        LogError("O_RDONLY | O_TRUNC, existing file: {}", status);
}

void TestFileOps(QFS &qfs)
{
    LogTest("File operations");

    int fd = qfs.Creat("/rwtest");
    Resolved r;
    qfs.Resolve("/rwtest", r);

    if (r.node == nullptr)
    {
        LogError("Didn't create /rwtest file");
        return;
    }
    auto size = &r.node->st.st_size;
    char buffer[1024];

    //

    const char *teststr = "First sentence. Second sentence.";
    const char *overw = "OVERWRITTEN";
    auto teststr_len = strlen(teststr);
    auto overw_len = strlen(overw);

    if (int bw = qfs.Write(fd, teststr, teststr_len); bw == teststr_len)
        LogSuccess("Written test string 1 to file.");
    else
        LogError("Didn't write test string 1: {} out of {}", bw, teststr_len);
    if (int bw = qfs.PWrite(fd, overw, overw_len, teststr_len - 5); bw == overw_len)
        LogSuccess("Written test string 1 to file.");
    else
        LogError("Didn't write test string 1: {} out of {}", bw, teststr_len);

    qfs.Close(fd);

    // Readback

    fd = qfs.Open("/rwtest", O_RDWR);
    memset(buffer, 0, 1024);
    if (int br = qfs.Read(fd, buffer, 1024); br == teststr_len + overw_len - 5)
        LogSuccess("Read test string:\n[START]{}[EOF]", buffer);
    else
        LogError("Didn't read test string 1: {} out of {}", br, teststr_len);

    if (int status = memcmp(buffer, teststr, teststr_len - 5); status != 0)
    {
        LogError("Invalid readback - first string");
    }
    if (int status = memcmp(buffer + teststr_len - 5, overw, overw_len); status != 0)
    {
        LogError("Invalid readback - overwriting string");
    }

    if (int status = qfs.FTruncate(fd, 15); status == 0 && *size == 15)
        LogSuccess("File truncated to {} bytes", *size);
    else
        LogError("Can't truncate: supposed to be 15, returned {}, size is {}", status, *size);

    qfs.Close(fd);
}

void TestFileSeek(QFS &qfs)
{
    LogTest("Seek");
    const char *seekstr = "ori+00.cur+36.end-49.filler.filler.filler.cur-13.ori+28.";
    int fd = qfs.Open("/seektest", O_CREAT | O_RDWR);
    qfs.Write(fd, seekstr, strlen(seekstr));

    char buf[32];

    qfs.LSeek(fd, 0, SeekOrigin::ORIGIN);
    if (6 != qfs.Read(fd, buf, 6))
        LogError("Can't read data for ORIGIN+00");
    TEST(strncmp(buf, "ori+00", 6) == 0, "Success: {}", "Fail: {}", "ORIGIN+00");

    qfs.LSeek(fd, 8, SeekOrigin::CURRENT);
    if (6 != qfs.Read(fd, buf, 6))
        LogError("Can't read data for CURRENT+08");
    TEST(strncmp(buf, "end-49", 6) == 0, "Success: {}", "Fail: {}", "CURRENT+08");

    qfs.LSeek(fd, -49, SeekOrigin::END);
    if (6 != qfs.Read(fd, buf, 6))
        LogError("Can't read data for END-49");
    TEST(strncmp(buf, "cur+36", 6) == 0, "Success: {}", "Fail: {}", "END-49");

    qfs.LSeek(fd, 36, SeekOrigin::CURRENT);
    if (6 != qfs.Read(fd, buf, 6))
        LogError("Can't read data for CURRENT+36");
    TEST(strncmp(buf, "ori+28", 6) == 0, "Success: {}", "Fail: {}", "CURRENT+36");

    qfs.LSeek(fd, -13, SeekOrigin::CURRENT);
    if (6 != qfs.Read(fd, buf, 6))
        LogError("Can't read data for CURRENT-10");
    TEST(strncmp(buf, "cur-13", 6) == 0, "Success: {}", "Fail: {}", "CURRENT-13");

    // no change to ptr (-100)
    TEST(int status = qfs.LSeek(fd, -100, SeekOrigin::ORIGIN); status == -QUASI_EINVAL, "EINVAL ({}) {}", "{} {}", status, "on ORIGIN -OOB");
    // 100
    TEST(int status = qfs.LSeek(fd, 100, SeekOrigin::ORIGIN); status == 100, "no error ({}) {}", "{} {}", status, "on ORIGIN +OOB");
    // no change to ptr (-100)
    TEST(int status = qfs.LSeek(fd, -200, SeekOrigin::CURRENT); status == -QUASI_EINVAL, "EINVAL ({}) {}", "{} {}", status, "on CURRENT -OOB");
    // 200
    TEST(int status = qfs.LSeek(fd, 100, SeekOrigin::CURRENT); status == 200, "no error ({}) {}", "{} {}", status, "on CURRENT +OOB");
    // no change to ptr (200)
    TEST(int status = qfs.LSeek(fd, -100, SeekOrigin::END); status == -QUASI_EINVAL, "EINVAL ({}) {}", "{} {}", status, "on END -OOB");
    // 100 + file size
    TEST(int status = qfs.LSeek(fd, 100, SeekOrigin::END); status == 100 + strlen(seekstr), "no error ({}) {}", "{} {}", status, "on END +OOB");
}

//
// Directories (I/O)
//

void TestDirOpen(QFS &qfs)
{
    LogTest("Dir open/close");

    Resolved r{};

    if (int status = qfs.Open("/dir_open", O_DIRECTORY); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent dir, O_READ | O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("nonexistent dir, O_READ | O_DIRECTORY: {}", status);

    if (int status = qfs.Open("/dir_open", O_DIRECTORY | O_WRONLY); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent dir, O_WRONLY | O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("nonexistent dir, O_WRONLY | O_DIRECTORY: {}", status);

    //
    qfs.MKDir("/dir_open");
    //

    if (int status = qfs.Open("/dir_open", O_DIRECTORY); 0 < status)
    {
        LogSuccess("existing dir, O_RDONLY | O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("existing dir, O_RDONLY | O_DIRECTORY: {}", status);

    if (int status = qfs.Open("/dir_open", 0); 0 < status)
    {
        LogSuccess("existing dir, O_RDONLY");
        qfs.Close(status);
    }
    else
        LogError("existing dir, O_RDONLY: {}", status);

    //
    // not implemented in host driver
    qfs.Creat("/notadir");
    //

    if (int status = qfs.Open("/notadir", O_DIRECTORY); -QUASI_ENOTDIR == status)
    {
        LogSuccess("not a dir, O_DIRECTORY");
        qfs.Close(status);
    }
    else
        LogError("not a dir, O_DIRECTORY: {}", status);
}

void TestDirOps(QFS &qfs) { UNIMPLEMENTED(); }
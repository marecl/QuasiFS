// INAA License @marecl 2025

#pragma once

#include "quasifs/quasifs_inode_directory.h"
#include "quasifs/quasifs_inode_regularfile.h"
#include "quasifs/quasifs_inode_symlink.h"
#include "quasifs/quasifs_partition.h"
#include "quasifs/quasifs.h"

#include "quasifs/quasi_sys_fcntl.h"

#include "../../dev/include/dev_std.h"
#include "log.h"

using namespace QuasiFS;

std::string file_mode(quasi_mode_t mode)
{
    std::string s;

    if (QUASI_S_ISREG(mode))
        s += '-';
    else if (QUASI_S_ISDIR(mode))
        s += 'd';
    else if (QUASI_S_ISLNK(mode))
        s += 'l';
    else if (QUASI_S_ISCHR(mode))
        s += 'c';
    else if (QUASI_S_ISBLK(mode))
        s += 'b';
    else if (QUASI_S_ISFIFO(mode))
        s += 'p';
    else if (QUASI_S_ISSOCK(mode))
        s += 's';
    else
        s += '?';

    // owner
    s += (mode & QUASI_S_IRUSR) ? 'r' : '-';
    s += (mode & QUASI_S_IWUSR) ? 'w' : '-';
    s += (mode & QUASI_S_IXUSR) ? 'x' : '-';

    // group
    s += (mode & QUASI_S_IRGRP) ? 'r' : '-';
    s += (mode & QUASI_S_IWGRP) ? 'w' : '-';
    s += (mode & QUASI_S_IXGRP) ? 'x' : '-';

    // other
    s += (mode & QUASI_S_IROTH) ? 'r' : '-';
    s += (mode & QUASI_S_IWOTH) ? 'w' : '-';
    s += (mode & QUASI_S_IXOTH) ? 'x' : '-';

    return s;
}

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
        auto st = node->st;
        char timebuf[64];
        std::tm *t = std::localtime(&st.st_mtime);
        std::strftime(timebuf, sizeof(timebuf), "%EY-%m-%d %H:%M", t);
        // TODO: UID/GID

        LogCustom("ls -la", "", "{} {:08} {:03d} {}:{} {:>08} {}\t{}{}", file_mode(st.st_mode), st.st_mode, st.st_nlink, /*st.st_uid*/ 0, /* st.st_gid*/ 0, st.st_size, timebuf, depEnt, name);
    }
    else
        depth--;

    if (node->is_link())
        LogCustom("ls -la", "", "\t\t\t\t\t\t\t\tsymlinked to ->{}", std::static_pointer_cast<Symlink>(node)->follow().string());

    if (node->is_dir())
    {
        if ("." == name)
            return;
        if (".." == name)
            return;

        auto dir = std::dynamic_pointer_cast<Directory>(node);
        if (dir->mounted_root)
        {
            LogCustom("ls -la", "", "\t\t\t\t\t\t\t\t|--{}{}", depEnt, "[MOUNTPOINT]");
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
void TestMountRO(QFS &qfs);

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
    qfs.Operation.MKDir("/mount");

    partition_ptr part = Partition::Create();
    qfs.Mount("/mount", part, MountOptions::MOUNT_RW);

    qfs.Operation.Creat("/mount/file");
}
void TestHostStat(QFS &qfs) UNIMPLEMENTED();

// /dev
void TestDev(QFS &qfs)
{
    LogTest("Test character devices");

    partition_ptr dev = Partition::Create();
    qfs.Operation.MKDir("/dev");
    qfs.Mount("/dev", dev, MountOptions::MOUNT_RW);
    uint8_t buffer[16];

    //
    // stdout
    //
    auto stdout_device = std::make_shared<Devices::DevStdout>();
    qfs.Operation.MKDir("/dev/fd"); // mimic real placement
    qfs.ForceInsert("/dev/fd", "1", stdout_device);
    qfs.Operation.LinkSymbolic("/dev/fd/1", "/dev/stdout");
    int stdout_fd = qfs.Operation.Open("/dev/stdout", QUASI_O_WRONLY);
    const char teststr[] = "\t\t\t\tIf you're reading this, stdout works as expected\n";
    auto teststr_len = strlen(teststr);
    if (int bw = qfs.Operation.Write(stdout_fd, teststr, teststr_len); bw == teststr_len)
        LogSuccess("Written {} bytes to stdout", bw);
    else
        LogError("Written {}/{} bytes to stdout", bw, teststr_len);
    qfs.Operation.Close(stdout_fd);

    //
    // /dev/random
    //
    auto random_device = std::make_shared<Devices::DevRandom>();
    qfs.ForceInsert("/dev", "random", random_device);
    int rand_fd = qfs.Operation.Open("/dev/random", QUASI_O_RDONLY);
    if (int br = qfs.Operation.Read(rand_fd, buffer, 16); 16 == br)
    {
        LogSuccess("Read {} bytes from /dev/random", br);
    }
    else
        LogError("Read {}/{} bytes from /dev/random", br, 16);
    for (auto i : buffer)
    {
        if (0xAA != i)
        {
            LogError("/dev/random didn't return \"random\" value");
            break;
        }
    }
    qfs.Operation.Close(rand_fd);

    //
    // End
    //
    qfs.Unmount("/dev");
}

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

    // /dev
    TestDev(qfs);

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
    Resolved res;

    if (int status = qfs.Operation.Creat(path); status >= 0)
        LogSuccess("Touched {}", path.string());
    else
        LogError("touch {} : {}", path.string(), status);

    if (int status = qfs.Resolve(path, res); status == 0)
        LogSuccess("Resolved");
    else
        LogError("Can't resolve: {}", status);

    if (res.parent != qfs.GetRoot())
        LogError("Wrong parent");

    if (int status = qfs.Operation.Unlink(path); status == 0)
        LogSuccess("Unlinked");
    else
        LogError("unlink failed: {}", status);

    if (int status = qfs.Resolve(path, res); status == -QUASI_ENOENT)
        LogSuccess("Removal confirmed");
    else
        LogError("file not removed: {}", status);
}

void TestMkRmdir(QFS &qfs)
{
    LogTest("Create a directory");
    fs::path path = "/testdir";
    Resolved res;

    if (int status = qfs.Operation.MKDir(path); status == 0)
        LogSuccess("mkdir'd {}", path.string());
    else
        LogError("mkdir {} : {}", path.string(), status);

    if (int status = qfs.Resolve(path, res); status == 0)
        LogSuccess("Resolved");
    else
        LogError("Can't resolve: {}", status);

    if (int status = qfs.Resolve("/testdir", res); status == 0)
        LogSuccess("Resolved /testdir");
    else
        LogError("Can't resolve: {}", status);

    if (int status = qfs.Resolve("/testdir/", res); status == 0)
        LogSuccess("Resolved /testdir/");
    else
        LogError("Can't resolve /testdir/ : {}", status);

    if (res.parent != qfs.GetRoot())
        LogError("Wrong parent");
    auto q = res.parent;
    auto qw = res.node;
    auto qwe = qfs.GetRoot();

    if (int status = qfs.Operation.RMDir(path); status == 0)
        LogSuccess("rmdir'd");
    else
        LogError("rmdir failed");

    if (int status = qfs.Resolve(path, res); -QUASI_ENOENT == status)
        LogSuccess("Removal confirmed");
    else
        LogError("dir not removed: {}", status);

    qfs.Operation.MKDir("/dir1");
    path = "/dir1/dir2";

    if (int status = qfs.Operation.MKDir(path); status == 0)
        LogSuccess("mkdir'd {}", path.string());
    else
        LogError("mkdir {} : {}", path.string(), status);

    if (int status = qfs.Resolve(path, res); status == 0)
        LogSuccess("Resolved");
    else
        LogError("Can't resolve: {}", status);

    if (int status = qfs.Operation.RMDir(path); status == 0)
        LogSuccess("rmdir'd");
    else
        LogError("rmdir failed");

    if (int status = qfs.Resolve(path, res); -QUASI_ENOENT == status)
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
    }

    Resolved res;

    qfs.Operation.MKDir("/dummy");
    qfs.Operation.MKDir("/dummy/mount");

    if (int status = qfs.Resolve("/dummy/mount", res); status == 0)
        LogSuccess("mkdir'd /dummy/mount");
    else
    {
        LogError("Mountpoint dir not created: {}", status);
    }

    if (nullptr == res.parent)
        LogError("Parent doesn't exist");

    if (nullptr == res.node)
        LogError("Child doesn't exist");

    auto parent_from_root = res.parent->lookup("mount")->GetFileno();
    auto self_fileno = std::static_pointer_cast<Directory>(res.node)->lookup(".")->GetFileno();
    auto parent_fileno = std::static_pointer_cast<Directory>(res.node)->lookup("..")->GetFileno();

    Log("\tPre-mount relation of /dummy/mount: {} (parent), {} (parent from self), {} (self)", parent_from_root, parent_fileno, self_fileno);

    if (int status = qfs.Mount("/dummy/mount", part); status == 0)
        LogSuccess("Mounted");
    else
        LogError("Can't mount: {}", status);

    if (int status = qfs.Resolve("/dummy/mount", res); status == 0)
        LogSuccess("After-mount resolved");
    else
        LogError("Can't resolve /dummy/mount after mounting: {}", status);

    if (nullptr == res.node)
    {
        LogError("Post-mount dir not found");
    }

    if (res.mountpoint != part)
    {
        LogError("Bad partition");
    }

    qfs.Resolve("/dummy", res);
    auto parent_from_root_mount = std::static_pointer_cast<Directory>(res.node)->lookup("mount")->GetFileno();

    qfs.Resolve("/dummy/mount", res);
    auto self_fileno_mount = std::static_pointer_cast<Directory>(res.node)->lookup(".")->GetFileno();
    auto parent_fileno_mount = std::static_pointer_cast<Directory>(res.node)->lookup("..")->GetFileno();

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

    if (res.node != part->GetRoot())
    {
        LogError("Mountpoint root isn't partition root");
    }

    if (int status = qfs.Unmount("/dummy/mount"); 0 == status)
        LogSuccess("Unmounted");
    else
        LogError("Can't unmount: {}", status);

    qfs.Operation.RMDir("/dummy/mount");
    qfs.Operation.RMDir("/dummy");
}

void TestMountFileRetention(QFS &qfs)
{
    LogTest("File retention/substitution on remount");

    auto part = Partition::Create();

    qfs.Operation.MKDir("/mount");

    qfs.Operation.Creat("/mount/dummy1"); // dummy files to shift fileno
    qfs.Operation.Creat("/mount/dummy2");
    qfs.Operation.Creat("/mount/dummy3");
    qfs.Operation.Creat("/mount/dummy4");
    qfs.Operation.Creat("/mount/testfile");
    qfs.Operation.MKDir("/mount/testdir");

    Resolved rfile{};
    Resolved rdir{};

    if (int status_file = qfs.Resolve("/mount/testfile", rfile); 0 == status_file)
        LogSuccess("Test file created");
    else
    {
        LogError("Test file not created: {}", status_file);
    }

    if (int status_dir = qfs.Resolve("/mount/testdir", rdir); 0 == status_dir)
        LogSuccess("Test directory created");
    else
    {
        LogError("Test directory not created: {}", status_dir);
    }

    auto ffileno = rfile.node->GetFileno();
    auto dfileno = rdir.node->GetFileno();
    Log("\tPre-mount file fileno: {}", ffileno);
    Log("\tPre-mount dir fileno: {}", dfileno);

    if (int status = qfs.Mount("/mount", part, MountOptions::MOUNT_RW); 0 == status)
        LogSuccess("Mounted /mount");
    else
        LogError("Can't mount /mount : {}", status);

    if (int status_file = qfs.Resolve("/mount/testfile", rfile); 0 == status_file && -QUASI_ENOENT == status_file)
    {
        LogError("Pre-mount file preserved (if 0), or error (if not ENOENT): {}", status_file);
    }
    if (int status_dir = qfs.Resolve("/mount/testdir", rdir); 0 == status_dir && -QUASI_ENOENT == status_dir)
    {
        LogError("Pre-mount directory preserved (if 0), or error (if not ENOENT): {}", status_dir);
    }

    qfs.Operation.Creat("/mount/testfile_mnt");
    qfs.Operation.MKDir("/mount/testdir_mnt");

    if (int status_file = qfs.Resolve("/mount/testfile_mnt", rfile); 0 == status_file)
        LogSuccess("Test file created (mount)");
    else
    {
        LogError("After-mount file not created: {}", status_file);
    }

    if (int status_dir = qfs.Resolve("/mount/testdir_mnt", rdir); 0 == status_dir)
        LogSuccess("Test dir created (mount)");
    else
    {
        LogError("After-mount directory not created: {}", status_dir);
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

void TestMountRO(QFS &qfs)
{
    partition_ptr ropart = Partition::Create();
    const char *rostr = "RO PREEXISTING CONTENT";
    auto rostr_len = strlen(rostr);

    qfs.Operation.MKDir("/ro");

    if (int mount_status = qfs.Mount("/ro", ropart, MountOptions::MOUNT_RW); mount_status == 0)
        LogSuccess("Mounted /ro as RW partition");
    else
        LogError("Can't mount /ro as RW: {}", mount_status);

    qfs.Operation.MKDir("/ro/roo");

    int existing_fd = qfs.Operation.Creat("/ro/exist");
    if (int bw = qfs.Operation.Write(existing_fd, rostr, rostr_len); bw != rostr_len)
        LogError("Can't write test data: {} out of {} written", bw, rostr_len);
    qfs.Operation.Close(existing_fd);

    if (int mount_status = qfs.Mount("/ro", ropart, MountOptions::MOUNT_NOOPT | MountOptions::MOUNT_REMOUNT); mount_status == 0)
        LogSuccess("Mounted /ro as read-only partition");
    else
        LogError("Can't mount /ro as RO: {}", mount_status);

    if (int fd = qfs.Operation.Creat("/ro/bad"); fd == -QUASI_EROFS)
        LogSuccess("Can't create a file in RO partition");
    else
    {
        LogError("Unexpected return from creat(): {}", fd);
        Resolved res;
        if (int resolve_status = qfs.Resolve("/ro/bad", res); resolve_status == 0)
            LogError("Created a file in RO partition");
    }

    int fd = qfs.Operation.Open("/ro/exist", 0);
    if (fd >= 0)
        LogSuccess("Preexisting file open. fd={}", fd);
    else
        LogError("Can't open file. Error: {}", fd);
    char buffer[128]{0};
    int rd = qfs.Operation.Read(fd, buffer, 128);

    if (rd >= 0)
        LogSuccess("Success: {} bytes read from preexisting file", rd);
    else
        LogError("Error: {} bytes read from preexisting file", rd);

    if (memcmp(rostr, buffer, rd) == 0)
        LogSuccess("Contents verified successfully");
    else
        LogError("Failed to verify contents");
}

//
// Links
//

void TestStLinkFile(QFS &qfs)
{
    LogTest("File link counter");

    Resolved res;

    qfs.Operation.Creat("/file");
    qfs.Resolve("/file", res);

    file_ptr f = std::reinterpret_pointer_cast<RegularFile>(res.node);

    auto nlink = &f->st.st_nlink;

    if (1 == *nlink)
        LogSuccess("Created new file. nlink=1", *nlink);
    else
        LogError("New file link {} != 1", *nlink);

    if (int status = qfs.Operation.Link("/file", "/file2"); 0 == status)
        LogSuccess("File linked");
    else
        LogError("Link existing file failed");

    if (2 == *nlink)
        LogSuccess("nlink ==2");
    else
        LogError("New file link {} != 2", *nlink);

    if (int status = qfs.Operation.Unlink("/file"); 0 == status)
        LogSuccess("File unlinked");
    else
        LogError("Unlink OG file failed: {}", status);

    if (1 == *nlink)
        LogSuccess("nlink ==1");
    else
        LogError("New file link {} != 1", *nlink);

    if (int status = qfs.Operation.Unlink("/file2"); 0 == status)
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

    Resolved res;

    qfs.Operation.MKDir("/dir");
    qfs.Resolve("/dir", res);

    dir_ptr f = std::reinterpret_pointer_cast<Directory>(res.node);

    auto nlink = &f->st.st_nlink;

    if (2 == *nlink)
        LogSuccess("Created new file. nlink=2", *nlink);
    else
        LogError("New dir link {} != 2", *nlink);

    if (int status = qfs.Operation.Link("/dir", "/dir2"); -QUASI_EPERM == status)
        LogSuccess("Dir can't be linked");
    else
        LogError("Dir linked: {}", status);

    if (int status = qfs.Operation.MKDir("/dir/dir2"); 0 == status)
        LogSuccess("Subdir created");
    else
        LogError("Can't create subdir");

    if (3 == *nlink)
        LogSuccess("Dir with subdir nlink==3");
    else
        LogError("Dir with subdir nlink {} != 3", *nlink);

    if (int status = qfs.Operation.RMDir("/dir/dir2"); 0 == status)
        LogSuccess("subdir unlinked");
    else
        LogError("Unlink subdir failed: {}", status);

    if (2 == *nlink)
        LogSuccess("Dir nlink decreased after removing subdir nlink==2");
    else
        LogError("Dir nlink didn't decrease after removing subdir nlink: {} != 2", *nlink);

    if (int status = qfs.Operation.RMDir("/dir"); 0 == status)
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

    qfs.Operation.Close(qfs.Operation.Creat("/link_source_file"));
    qfs.Operation.MKDir("/oneup");
    qfs.Operation.Close(qfs.Operation.Creat("/oneup/link_source_file_up"));

    Resolved res;

    int resolve_status = qfs.Resolve("/link_source_file", res);
    const file_ptr src_file = std::static_pointer_cast<RegularFile>(res.node);
    resolve_status = qfs.Resolve("/oneup/link_source_file_up", res);
    const file_ptr src_file_up = std::static_pointer_cast<RegularFile>(res.node);
    file_ptr dst_file;

    // link, same directory
    qfs.Operation.LinkSymbolic("/link_source_file", "/link_dest_file");
    resolve_status = qfs.Resolve("/link_dest_file", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);
    if (src_file == dst_file)
        LogSuccess("Same directory symlink works");
    else
        LogError("Same directory symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.Operation.LinkSymbolic("/oneup/link_source_file_up", "/oneup/link_dest_file_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_file_up", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);
    if (src_file_up == dst_file)
        LogSuccess("Same directory, 1 level deep symlink works");
    else
        LogError("Same directory 1 level deep symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.Operation.LinkSymbolic("/oneup/link_source_file_up", "/link_dest_file_to_down");
    resolve_status = qfs.Resolve("/link_dest_file_to_down", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);
    if (src_file_up == dst_file)
        LogSuccess("Same directory, up->down works");
    else
        LogError("Same directory up->down failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.Operation.LinkSymbolic("/link_source_file", "/oneup/link_dest_file_to_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_file_to_up", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);
    if (src_file == dst_file)
        LogSuccess("Same directory, down->up works");
    else
        LogError("Same directory down->up failed: Resolve returned {}", resolve_status);
}

void TestSymlinkDir(QFS &qfs)
{
    LogTest("Symlinking directories");

    qfs.Operation.MKDir("/link_source_dir");
    qfs.Operation.MKDir("/oneup");
    qfs.Operation.MKDir("/oneup/link_source_dir_up");

    Resolved res;

    int resolve_status = qfs.Resolve("/link_source_dir", res);
    const dir_ptr src_dir = std::static_pointer_cast<Directory>(res.node);
    resolve_status = qfs.Resolve("/oneup/link_source_dir_up", res);
    const dir_ptr src_dir_up = std::static_pointer_cast<Directory>(res.node);
    dir_ptr dst_dir;

    // link, same directory
    qfs.Operation.LinkSymbolic("/link_source_dir", "/link_dest_dir");
    resolve_status = qfs.Resolve("/link_dest_dir", res);
    dst_dir = std::static_pointer_cast<Directory>(res.node);
    if (src_dir == dst_dir)
        LogSuccess("Same directory symlink works");
    else
        LogError("Same directory symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.Operation.LinkSymbolic("/oneup/link_source_dir_up", "/oneup/link_dest_dir_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_dir_up", res);
    dst_dir = std::static_pointer_cast<Directory>(res.node);
    if (src_dir_up == dst_dir)
        LogSuccess("Same directory, 1 level deep symlink works");
    else
        LogError("Same directory 1 level deep symlink failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.Operation.LinkSymbolic("/oneup/link_source_dir_up", "/link_dest_dir_to_down");
    resolve_status = qfs.Resolve("/link_dest_dir_to_down", res);
    dst_dir = std::static_pointer_cast<Directory>(res.node);
    if (src_dir_up == dst_dir)
        LogSuccess("Same directory, up->down works");
    else
        LogError("Same directory up->down failed: Resolve returned {}", resolve_status);

    // link, same directory, 1 level deep
    qfs.Operation.LinkSymbolic("/link_source_dir", "/oneup/link_dest_dir_to_up");
    resolve_status = qfs.Resolve("/oneup/link_dest_dir_to_up", res);
    dst_dir = std::static_pointer_cast<Directory>(res.node);
    if (src_dir == dst_dir)
        LogSuccess("Same directory, down->up works");
    else
        LogError("Same directory down->up failed: Resolve returned {}", resolve_status);
}

void TestSymlinkCursed(QFS &qfs)
{
    LogTest("Cursed links");

    qfs.Operation.Creat("/src_file");
    qfs.Operation.MKDir("/layer1");
    qfs.Operation.MKDir("/layer1/layer2");
    qfs.Operation.MKDir("/layer1/layer2/layer3");
    qfs.Operation.MKDir("/layer1/layer2/layer3/layer4");

    Resolved res;
    int resolve_status = qfs.Resolve("/src_file", res);
    const file_ptr src_file = std::static_pointer_cast<RegularFile>(res.node);
    if (nullptr == src_file)
        LogError("Didn't create test file");
    file_ptr dst_file;

    // linking directories
    qfs.Operation.LinkSymbolic("/layer1/layer2/layer3/layer4", "/l4");
    qfs.Operation.LinkSymbolic("/src_file", "/l4/sos");
    resolve_status = qfs.Resolve("/l4/sos", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);

    if (src_file == dst_file)
        LogSuccess("File symlinked to symlinked directory tree");
    else
        LogError("File symlinked to symlinked directory tree: Resolve returned {}", resolve_status);

    // linking directories
    qfs.Operation.LinkSymbolic("/l4/sos", "/l4/sos2");
    resolve_status = qfs.Resolve("/l4/sos2", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);

    if (src_file == dst_file)
        LogSuccess("File symlinked to file symlinked in symlinked directory tree");
    else
        LogError("File symlinked to file symlinked in symlinked directory tree: Resolve returned {}", resolve_status);

    qfs.Operation.LinkSymbolic("/layer1/layer2/layer3/layer4", "/layer1/layer2/early4");
    qfs.Operation.LinkSymbolic("/src_file", "/layer1/layer2/early4/early_file");
    resolve_status = qfs.Resolve("/layer1/layer2/early4/early_file", res);
    dst_file = std::static_pointer_cast<RegularFile>(res.node);

    if (src_file == dst_file)
        LogSuccess("File symlinked to file symlinked in a directory symlinked within directory tree");
    else
        LogError("File symlinked to file symlinked in a directory symlinked within directory tree: Resolve returned {}", resolve_status);

    // loops
    qfs.Operation.MKDir("/tmp");
    qfs.Operation.LinkSymbolic("/tmp/loopLinkDir", "/tmp/loopLinkDir");
    resolve_status = qfs.Resolve("/tmp/loopLinkDir", res);
    if (-QUASI_ELOOP == resolve_status)
        LogSuccess("Symlink loop safeguard works for directories");
    else
        LogError("Symlink loop safeguard for directories didn't trigger: Resolve returned {}", resolve_status);

    qfs.Operation.LinkSymbolic("/tmp/loopLinkFile", "/tmp/loopLinkFile");
    resolve_status = qfs.Resolve("/tmp/loopLinkFile", res);
    if (-QUASI_ELOOP == resolve_status)
        LogSuccess("Symlink loop safeguard works for files");
    else
        LogError("Symlink loop safeguard for files didn't trigger: Resolve returned {}", resolve_status);

    qfs.Operation.Creat("/missing_file");
    qfs.Operation.LinkSymbolic("/missing_file", "/dangling_file");
    qfs.Operation.Unlink("/missing_file");
    resolve_status = qfs.Resolve("/dangling_file", res);

    if (-QUASI_ENOENT == resolve_status)
        LogSuccess("Dangling symlink returns ENOENT");
    else
        LogError("Resolving dangling symlink returns wrong error: {}", resolve_status);

    qfs.Operation.MKDir("/dir1");
    qfs.Operation.MKDir("/dir2");
    qfs.Operation.Creat("/dir2/XD");
    qfs.Operation.LinkSymbolic("/dir2", "/dir1/missing");

    resolve_status = qfs.Resolve("/dir1/missing/XD", res);
    if (0 == resolve_status)
        LogSuccess("Resolved file in directory symlinked inside a tree");
    else
        LogError("Didn't resolve file in directory symlinked inside a tree: ", resolve_status);

    if (0 != qfs.Operation.Unlink("/dir1/missing"))
    {
        LogError("Unlink should remove symlink when path is not preceeded with /");
    }

    resolve_status = qfs.Resolve("/dir1/missing/XD", res);

    if (-QUASI_ENOENT == resolve_status)
        LogSuccess("Dangling symlink inside a tree returns ENOENT");
    else
        LogError("Resolving dangling symlink inside a tree returns wrong error: {}", resolve_status);

    //
    qfs.Operation.LinkSymbolic("/dir2/XD", "/unlinked_link");
    if (0 != qfs.Operation.Unlink("/unlinked_link"))
    {
        LogError("Didn't unlink a symlink pointing at a file");
    }

    resolve_status = qfs.Resolve("/dir2/XD", res);

    if (0 == resolve_status)
        LogSuccess("Unlinking a symlink didn't touch linked file");
    else
        LogError("Unlinking a symlink removed the file it pointed to: {}", resolve_status);
}

//
// Files (I/O)
//

void TestFileOpen(QFS &qfs)
{
    LogTest("File open/close");

    Resolved res;

    if (int status = qfs.Operation.Open("/file_open", 0); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent file, QUASI_O_READ");
        qfs.Operation.Close(status);
    }
    else
        LogError("O_READ, nonexistent file with trailing /: {}", status);

    if (int status = qfs.Operation.Open("/file_open", QUASI_O_CREAT); 0 <= status)
    {
        LogSuccess("nonexistent file, QUASI_O_READ | QUASI_O_CREAT");
        qfs.Operation.Close(status);
    }
    else
        LogError("O_READ | QUASI_O_CREAT, nonexistent file: {}", status);

    //
    if (-QUASI_ENOENT == qfs.Resolve("/file_open", res))
        LogError("O_CREAT didn't create the file");
    //

    if (int status = qfs.Operation.Open("/file_open/", 0); -QUASI_ENOTDIR == status)
    {
        LogSuccess("nonexistent file with trailing /, QUASI_O_READ");
        qfs.Operation.Close(status);
    }
    else
        LogError("O_READ, nonexistent file with trailing /: {}", status);

    if (int status = qfs.Operation.Open("/file_open", QUASI_O_RDWR); 0 <= status)
    {
        LogSuccess("existing file, QUASI_O_RDWR");
        qfs.Operation.Close(status);
    }
    else
        LogError("O_RDWR, existing file: {}", status);

    // Edge case - undefined in POSIX
    // We can open the file for reading only and truncate it anyway
    if (int status = qfs.Operation.Open("/file_open", QUASI_O_RDONLY | QUASI_O_TRUNC); 0 <= status)
    {
        LogSuccess("existing file, QUASI_O_RDONLY | QUASI_O_TRUNC");
        qfs.Operation.Close(status);
    }
    else
        LogError("O_RDONLY | QUASI_O_TRUNC, existing file: {}", status);
}

void TestFileOps(QFS &qfs)
{
    LogTest("File operations");

    int fd = qfs.Operation.Creat("/rwtest");
    Resolved res;
    qfs.Resolve("/rwtest", res);

    if (nullptr == res.node)
    {
        LogError("Didn't create /rwtest file");
    }
    auto size = &res.node->st.st_size;
    char buffer[1024];

    //

    const char *teststr = "First sentence. Second sentence.";
    const char *overw = "OVERWRITTEN";
    auto teststr_len = strlen(teststr);
    auto overw_len = strlen(overw);

    if (int bw = qfs.Operation.Write(fd, teststr, teststr_len); bw == teststr_len)
        LogSuccess("Written test string 1 to file.");
    else
        LogError("Didn't write test string 1: {} out of {}", bw, teststr_len);
    if (int bw = qfs.Operation.PWrite(fd, overw, overw_len, teststr_len - 5); bw == overw_len)
        LogSuccess("Written test string 1 to file.");
    else
        LogError("Didn't write test string 1: {} out of {}", bw, teststr_len);

    qfs.Operation.Close(fd);

    // Readback

    fd = qfs.Operation.Open("/rwtest", QUASI_O_RDWR);
    memset(buffer, 0, 1024);
    if (int br = qfs.Operation.Read(fd, buffer, 1024); teststr_len + overw_len - 5 == br)
        LogSuccess("Read test string:\n\t\t\t\t[START]{}[EOF]", buffer);
    else
        LogError("Didn't read test string 1: {} out of {}", br, teststr_len);

    if (int status = memcmp(buffer, teststr, teststr_len - 5); 0 != status)
    {
        LogError("Invalid readback - first string");
    }
    if (int status = memcmp(buffer + teststr_len - 5, overw, overw_len); 0 != status)
    {
        LogError("Invalid readback - overwriting string");
    }

    if (int status = qfs.Operation.FTruncate(fd, 15); 0 == status && 15 == *size)
        LogSuccess("File truncated to {} bytes", *size);
    else
        LogError("Can't truncate: supposed to be 15, returned {}, size is {}", status, *size);

    qfs.Operation.Close(fd);
}

void TestFileSeek(QFS &qfs)
{
    LogTest("Seek");
    const char *seekstr = "ori+00.cur+36.end-49.filler.filler.filler.cur-13.ori+28.";
    int fd = qfs.Operation.Open("/seektest", QUASI_O_CREAT | QUASI_O_RDWR);
    qfs.Operation.Write(fd, seekstr, strlen(seekstr));

    char buf[32];

    qfs.Operation.LSeek(fd, 0, SeekOrigin::ORIGIN);
    if (6 != qfs.Operation.Read(fd, buf, 6))
        LogError("Can't read data for ORIGIN+00");
    TEST(strncmp(buf, "ori+00", 6) == 0, "Success: {}", "Fail: {}", "ORIGIN+00");

    qfs.Operation.LSeek(fd, 8, SeekOrigin::CURRENT);
    if (6 != qfs.Operation.Read(fd, buf, 6))
        LogError("Can't read data for CURRENT+08");
    TEST(strncmp(buf, "end-49", 6) == 0, "Success: {}", "Fail: {}", "CURRENT+08");

    qfs.Operation.LSeek(fd, -49, SeekOrigin::END);
    if (6 != qfs.Operation.Read(fd, buf, 6))
        LogError("Can't read data for END-49");
    TEST(strncmp(buf, "cur+36", 6) == 0, "Success: {}", "Fail: {}", "END-49");

    qfs.Operation.LSeek(fd, 36, SeekOrigin::CURRENT);
    if (6 != qfs.Operation.Read(fd, buf, 6))
        LogError("Can't read data for CURRENT+36");
    TEST(strncmp(buf, "ori+28", 6) == 0, "Success: {}", "Fail: {}", "CURRENT+36");

    qfs.Operation.LSeek(fd, -13, SeekOrigin::CURRENT);
    if (6 != qfs.Operation.Read(fd, buf, 6))
        LogError("Can't read data for CURRENT-10");
    TEST(strncmp(buf, "cur-13", 6) == 0, "Success: {}", "Fail: {}", "CURRENT-13");

    // no change to ptr (-100)
    TEST(int status = qfs.Operation.LSeek(fd, -100, SeekOrigin::ORIGIN); -QUASI_EINVAL == status, "EINVAL ({}) {}", "{} {}", status, "on ORIGIN -OOB");
    // 100
    TEST(int status = qfs.Operation.LSeek(fd, 100, SeekOrigin::ORIGIN); 100 == status, "no error ({}) {}", "{} {}", status, "on ORIGIN +OOB");
    // no change to ptr (-100)
    TEST(int status = qfs.Operation.LSeek(fd, -200, SeekOrigin::CURRENT); -QUASI_EINVAL == status, "EINVAL ({}) {}", "{} {}", status, "on CURRENT -OOB");
    // 200
    TEST(int status = qfs.Operation.LSeek(fd, 100, SeekOrigin::CURRENT); 200 == status, "no error ({}) {}", "{} {}", status, "on CURRENT +OOB");
    // no change to ptr (200)
    TEST(int status = qfs.Operation.LSeek(fd, -100, SeekOrigin::END); -QUASI_EINVAL == status, "EINVAL ({}) {}", "{} {}", status, "on END -OOB");
    // 100 + file size
    TEST(int status = qfs.Operation.LSeek(fd, 100, SeekOrigin::END); 100 + strlen(seekstr) == status, "no error ({}) {}", "{} {}", status, "on END +OOB");
}

//
// Directories (I/O)
//

void TestDirOpen(QFS &qfs)
{
    LogTest("Dir open/close");

    Resolved res;

    if (int status = qfs.Operation.Open("/dir_open", QUASI_O_DIRECTORY); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent dir, QUASI_O_READ | QUASI_O_DIRECTORY");
        qfs.Operation.Close(status);
    }
    else
        LogError("nonexistent dir, QUASI_O_READ | QUASI_O_DIRECTORY: {}", status);

    if (int status = qfs.Operation.Open("/dir_open", QUASI_O_DIRECTORY | QUASI_O_WRONLY); -QUASI_ENOENT == status)
    {
        LogSuccess("nonexistent dir, QUASI_O_WRONLY | QUASI_O_DIRECTORY");
        qfs.Operation.Close(status);
    }
    else
        LogError("nonexistent dir, QUASI_O_WRONLY | QUASI_O_DIRECTORY: {}", status);

    //
    qfs.Operation.MKDir("/dir_open");
    //

    if (int status = qfs.Operation.Open("/dir_open", QUASI_O_DIRECTORY); 0 < status)
    {
        LogSuccess("existing dir, QUASI_O_RDONLY | QUASI_O_DIRECTORY");
        qfs.Operation.Close(status);
    }
    else
        LogError("existing dir, QUASI_O_RDONLY | QUASI_O_DIRECTORY: {}", status);

    if (int status = qfs.Operation.Open("/dir_open", 0); 0 < status)
    {
        LogSuccess("existing dir, QUASI_O_RDONLY");
        qfs.Operation.Close(status);
    }
    else
        LogError("existing dir, QUASI_O_RDONLY: {}", status);

    //
    // not implemented in host driver
    qfs.Operation.Creat("/notadir");
    //

    if (int status = qfs.Operation.Open("/notadir", QUASI_O_DIRECTORY); -QUASI_ENOTDIR == status)
    {
        LogSuccess("not a dir, QUASI_O_DIRECTORY");
        qfs.Operation.Close(status);
    }
    else
        LogError("not a dir, QUASI_O_DIRECTORY: {}", status);
}

void TestDirOps(QFS &qfs) { UNIMPLEMENTED(); }
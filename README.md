# QuasiFS

Miniature UNIX-like virtual filesystem.

## Objects

### Superblock wrapper

Main `QFS` class, handling partitions and interactions between them.
Contains open file descriptors, mounted partitions (with info) and acts as the highest root for path resolution.

### Partitions

Standalone containers. Virtual and host-mapped, mix and match!
Can be mounted inside directories with `QFS`.
Using partition directly skips most of the access checks, but still validates node locations.
It's also possible to resolve path within the partition.
There are "low-level" `touch`, `mkdir`, `link` and `unlink` to perform basic tasks. The rest is

### Directories

Storage for anything inheriting `Inode`.
Directories can have a `Partition` mounted inside of them.

### Regular files

Virtual files have `std::vector` as data storage.
Same as with real files, all file operations are supported.
State of the file is tracked by `QFS`, including size and file pointer location.

### Symlinks

Soft-links - full path to an arbitrary element.
If either source or destination is in host-bound partition, link will fail.

### Inodes

Raw item entry on disk, parent class for all disk entries.
They can be used as a base to create customized disk entries or virtual devices.

## Functions

Supported operations are to be the same as UNIX would handle them.
QFS has a special member `Operation` with FS-specific operations.
QFS-specifix methods are available directly.

### Mount

Mount a `Partition` in a `Directory`.
Files present in `Directory` are preserved, with `Partition` being favoured on mountpoint resolution.
Currently supported mount modes are `RW` and `RO`, simulating real life `mount`.

### File operations

Nearly all standard file operations are implemented.
POSIX methods are available only through `Operation` member namespace-like-class.

### Directory operations

[WIP]

### Links and symlinks

Hard-links and symlinks to simplify access.
Hardlinks are available only in scope of the local partition.
Symlinks can be used on any partition with any target, as long as their resolution is done by `QFS`.
It's possible to use symlinks only within one partition, but the behaviour is neither documented nor tracked.

### Permissions

Files and directories may have UNIX-like permissions set.
Path resolution, file creation and r/w ops depend on permissions, so they may produce expected results.
Permissions are verified **only** when operating in QFS.  
Low-level access directly to inodes is **not** recommended.
You may semi-freely roam in `Partition`, but you can mess things up there too.

## Precautions

This is a WIP project.
Use QFS for maximum compatibility with host FS, although results may vary.
Using Partitions (and lower) may or may produce undefined behaviour.

## Host integration

It's now possible to bind host's directories to QFS Partition.
By design, QFS doesn't expect external changes. There is *no* desync detection or repair procedure. Your FS will just start breaking.
Experimental feature, proceed with caution.
This makes QFS have obfuscated access to *virtually any* location on your machine.
There may (or may not) be safeguards against breaking out from specified path.
Use at your own risk.

Virtual files are not written to when in host-bound partition. QFS tracks `st_size` and file pointer position independently from host.

### Usage (preliminary)

Target partition must be created with *absolute* host path in constructor.
At the time it must be mounted somewhere in QFS to use `SyncHost()`.
This will populate inodes in QFS, and is expected to be run only **once**. Otherwise the behaviour is undefined.

## Notes

### General

Only QFS is made to work with host directories. Using Partition (and deeper) may produce unexpected results.

### Behaviour

When using host-bound partitions, root directory *must* exist on host filesystem.
If a partition is mounted in a directory bound to host, its contents are **invisible** from host's perspective.
Similarly to virtual mounts, preexisting directory contents are ignored on path resolution.

### Permissions

Permissions are set by hand in standard octal format (like `0755`).
There are no "users" implemented, so all permissions are OR-ed. This means all groups are merged, so `0644` will turn into an equivalent of `0666`, `0421` into `0777` and so on.
Permissions *should theoretically* be respected by path resolution and file operations, but there are no tests for that yet.

### Safeguards for host-bound partitions

`QUASI_EPERM` on accessing path on lower level than assigned to the partition.
Example: `Partition` is assigned to `/home/user/application/filesystem`. Application has access to everything in that folder, which is referred to as root, i.e. `/` operates in root of the assigned directory.
App may try to open malicious locations like `../../.bashrc` to have access to user files. This call would (should) be caught, since `/home/user` is below assigned path.

## Quickstart

### Purely virtual

```
#include "quasifs/quasifs.h"

int main() {
    QFS qfs;

    qfs.Operation.MKDir("/directory");
    int fd = qfs.Operation.Creat("/directory/file");
    qfs.Operation.Close(fd);

    printTree(qfs.GetRoot(), "/", 0);
}
```

### Host-bound root
You can pass host directory path in `QFS` constructor.
This will place entire root in host-bound directory.
If there are mountpoints within that directory, their contents will not be visible.

```
#include "quasifs/quasifs.h"

int main() {
    // make sure this directory exists
    QFS qfs("host");

    // optional, imports pre-existing files to QFS
    qfs.SyncHost();

    qfs.Operation.MKDir("/directory");
    int fd = qfs.Operation.Creat("/directory/file");
    qfs.Operation.Close(fd);

    printTree(qfs.GetRoot(), "/", 0);
}
```

### Mountpoints
You can pass host directory path in `Partition` constructor.

```
#include "quasifs/quasifs.h"

int main() {
    QFS qfs;

    partition_ptr virtual_partition = Partition::Create();
    partition_ptr ro_partition = Partition::Create();
    partition_ptr host_partition = Partition::Create("host");

    // this skips permission checks, so it will be accessible
    ro_partition->touch<RegularFile>(ro_partition->GetRoot(), "rofile_old");
    ro_partition->touch(ro_partition->GetRoot(), "rofile_different_old", RegularFile::Create());

    qfs.Operation.MKDir("/vdir");
    qfs.Operation.MKDir("/ro");
    qfs.Operation.MKDir("/hdir");

    qfs.Mount("/vdir", virtual_partition, MountOptions::MOUNT_RW);
    qfs.Mount("/ro", ro_partition);
    qfs.Mount("/hdir", host_partition, MountOptions::MOUNT_RW);

    qfs.Operation.Creat("/vdir/vfile");
    qfs.Operation.Creat("/ro/rofile_new");  // this will fail
    qfs.Operation.Creat("/hdir/hfile");

    printTree(qfs.GetRoot(), "/", 0);
}
```
### Other
To see other examples, see tests. They contain (some) comments on how things work, as well as expected values.
I'm trying to make them as exhaustive as possible to mimic original behaviour.

# TODO

* Missing tests:
  * Path resolution
  * Mixed hardlinks (partitions, trees etc.)
  * Directory operations (waiting for dirents)
* Dirents
* Add more file types (block, char, etc.)
* File timestamps
* Don't remove inode if a file is open, but nlink reaches 0
* Flexible (and realistic) fileno and blockdev values

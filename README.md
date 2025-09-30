# QuasiFS
Miniature UNIX-like virtual filesystem.

## Objects

### Superblock wrapper
Main `QFS` class, handling partitions and interactions between them.  
Contains open file descriptors, mounted partitions (with info) and acts as the highest root for path resolution.  

### Partitions
Standalone containers. Virtual and host-mapped, mix and match!
Can be mounted inside directories with `QFS`.

### Directories
Storage for anything inheriting `Inode`.  
Directories can have a `Partition` mounted inside of them.

### Regular files
Virtual files have `std::vector` as data storage.  
Same as with real files, all file operations are supported.  
State of the file is tracked by `QFS`, including size and file pointer location.

### Symlinks
Soft-links - full path to an arbitrary element.  

### Inodes
Raw item entry on disk, parent class for all disk entries.  
They can be used as a base to create customized disk entries or virtual devices.

## Functions
Supported operations are to be the same as UNIX would handle them.

### Mount
Mount a `Partition` in a `Directory`.  
Files present in `Directory` are preserved, with `Partition` being favoured on mountpoint resolution.
Currently supported mount modes are `RW` and `RO`, simulating real life `mount`. 

### File operations
Nearly all standard file operations are implemented.  

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

# TODO
* Tests for permissions
    * Path resolution
    * File operations (R/W/A)
    * Directory creation
* Missing tests:
    * Path resolution
    * Mixed hardlinks (partitions, trees etc.)
    * Directory operations (waiting for dirents)
* Dirents
* Add more file types (block, char, etc.)  
* File timestamps  
* Don't remove inode if a file is open, but nlink reaches 0
* Flexible (and realistic) fileno and blockdev values

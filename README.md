# QuasiFS
Miniature UNIX-like virtual filesystem.

## Objects

### Superblock wrapper
Main `QFS` class, handling partitions and interactions between them.

### Partitions
Standalone containers. Virtual and host-mapped, mix and match!

### Directories
Storage for anything inheriting `Inode`.  

### Regular files

### Symlinks
Soft-links - pointer to a location with absolute path.  

### Inodes
Raw item entry on disk, parent class for all disk entries.  
Can be used as a base for custom entries, for example `/dev/null`.

## Functions
Supported operations are to be the same as UNIX would handle them.

### Mount
Mount a `Partition` in a `Directory`.  
Files present in `Directory` are preserved, with `Partition` being favoured on mountpoint resolution.

### File operations
Beloved `touch`, `mkdir` and more.

### Links and symlinks
Hard-links and symlinks to simplify access.  
Hardlinks are available only in scope of the local partition.  
Symlinks are cross-partition, but handled only by `QFS`.  

## Precautions
This is a WIP project.  
Use QFS for maximum compatibility with host FS, although results may vary.  
Using Partitions (and lower) may or may not produce undefined behaviour.  

## Host integration
It's now possible to bind host's directories to QFS Partition.  
By design, QFS doesn't expect external changes. There is *no* desync detection or repair procedure. Your FS will just start breaking.
Experimental feature, proceed with caution.
This makes QFS have obfuscated access to *virtually any* location on your machine.  
There may (or may not) be safeguards against breaking out from specified path.  
Use at your own risk.

### Usage (preliminary)
Target partition must be created with *absolute* host path in constructor.
At the time it must be mounted somewhere in QFS to use `SyncHost()`.  
This will populate inodes in QFS, and is expected to be run only **once**. Otherwise the behaviour is undefined.

### Note
Only QFS is made to work with host directories. Using Partition (and deeper) may produce unexpected results.  
Proceed with caution when using host-bound files.  

### Behaviour
Partition root directory *must* exist on host filesystem.  
If a partition is mounted in a directory bound to host, its contents are **invisible** from host's perspective.  
Similarly to virtual mounts, preexisting directory contents are ignored on path resolution.

### Current safeguards
`QUASI_EPERM` on accessing path on lower level than assigned to the partition.
Example: `Partition` is assigned to `/home/user/application/filesystem`. Application has access to everything in that folder, which is referred to as root, i.e. `/` operates in root of the assigned directory.
App may try to open malicious locations like `../../.bashrc` to have access to user files. This call would (should) be caught, since `/home/user` is below assigned path.

# TODO
* Add more file types (block, char, etc.)  
* File timestamps  
* Don't remove inode if a file is open, but nlink reaches 0
* Finish drivers
* Permissions  
* Flexible (and realistic) fileno and blockdev values
* Dirents
* More tests

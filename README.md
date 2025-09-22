# QuasiFS
Miniature UNIX-like virtual filesystem.

## Objects

### Superblock wrapper
Main `QFS` class, handling partitions and interactions between them.

### Partitions
Standalone containers.

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

## Host integration
It's now possible to bind host's directories to QFS Partition.  
Experimental feature, proceed with caution.
This makes QFS have obfuscated access to *virtually any* location on your machine.  
There may (or may not) be safeguards against breaking out from specified path.  
Use at your own risk.

### Current safeguards
`QUASI_EPERM` on accessing path on lower level than assigned to the partition.
Example: `Partition` is assigned to `/home/user/application/filesystem`. Application has access to everything in that folder, which is referred to as root, i.e. `/` operates in root of the assigned directory.
App may try to open malicious locations like `../../.bashrc` to have access to user files. This call would (should) be caught, since `/home/user` is below assigned path.

# TODO
* Add more file types (block, char, etc.)  
* File timestamps  
* [WIP] Host-mounted files  
    * Normalize paths (remove relative operators)
    * Add top-level limit path (prohibit going beyond a certain path)
* Permissions  
* Flexible (and realistic) fileno and blockdev values
* Dirents
* More tests

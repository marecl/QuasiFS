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

# TODO
* Add more file types (block, char, etc.)  
* File timestamps  
* Host-mounted files  
* Permissions  
* Flexible (and realistic) fileno and blockdev values
* Dirents
* More tests
    * tracking `nlink`
    * verification of removed inodes  

// INAA License @marecl 2025

#pragma once

#include "quasi_types.h"

#define QUASI_S_IFMT 00170000
#define QUASI_S_IFSOCK 0140000
#define QUASI_S_IFLNK 0120000
#define QUASI_S_IFREG 0100000
#define QUASI_S_IFBLK 0060000
#define QUASI_S_IFDIR 0040000
#define QUASI_S_IFCHR 0020000
#define QUASI_S_IFIFO 0010000
#define QUASI_S_ISUID 0004000
#define QUASI_S_ISGID 0002000
#define QUASI_S_ISVTX 0001000

#define QUASI_S_ISLNK(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFLNK)
#define QUASI_S_ISREG(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFREG)
#define QUASI_S_ISDIR(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFDIR)
#define QUASI_S_ISCHR(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFCHR)
#define QUASI_S_ISBLK(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFBLK)
#define QUASI_S_ISFIFO(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFIFO)
#define QUASI_S_ISSOCK(m) (((m) & QUASI_S_IFMT) == QUASI_S_IFSOCK)

#define QUASI_S_IRWXU 00700
#define QUASI_S_IRUSR 00400
#define QUASI_S_IWUSR 00200
#define QUASI_S_IXUSR 00100

#define QUASI_S_IRWXG 00070
#define QUASI_S_IRGRP 00040
#define QUASI_S_IWGRP 00020
#define QUASI_S_IXGRP 00010

#define QUASI_S_IRWXO 00007
#define QUASI_S_IROTH 00004
#define QUASI_S_IWOTH 00002
#define QUASI_S_IXOTH 00001

namespace QuasiFS
{
    namespace Stat
    {

        struct statx_timestamp
        {
            __s64 tv_sec;
            __u32 tv_nsec;
            __s32 __reserved;
        };

        struct quasi_stat_t
        {
            quasi_dev_t st_dev;     /* Device.  */
            quasi_ino_t st_ino;     /* File serial number.	*/
            quasi_nlink_t st_nlink; /* Link count.  */
            quasi_mode_t st_mode;   /* File mode.  */
                                    //   __uid_t st_uid;		/* User ID of the file's owner.	*/
                                    //   __gid_t st_gid;		/* Group ID of the file's group.*/
                                    //   __dev_t st_rdev;		/* Device number, if device.  */
            quasi_off_t st_size;    /* Size of file, in bytes.  */
            blksize_t st_blksize;   /* Optimal block size for I/O.  */
            blkcnt_t st_blocks;     /* Number 512-byte blocks allocated. */

            struct timespec st_atim; /* Time of last access.  */
            struct timespec st_mtim; /* Time of last modification.  */
            struct timespec st_ctim; /* Time of last status change.  */
#define st_atime st_atim.tv_sec      /* Backward compatibility.  */
#define st_mtime st_mtim.tv_sec
#define st_ctime st_ctim.tv_sec
        };
    }
}
// INAA License @marecl 2025

#pragma once

#define QUASI_O_ACCMODE 0003
#define QUASI_O_RDONLY 00
#define QUASI_O_WRONLY 01
#define QUASI_O_RDWR 02
#define QUASI_O_CREAT 0100  /* Not fcntl.  */
#define QUASI_O_EXCL 0200   /* Not fcntl.  */
#define QUASI_O_NOCTTY 0400 /* Not fcntl.  */
#define QUASI_O_TRUNC 01000 /* Not fcntl.  */
#define QUASI_O_APPEND 02000
#define QUASI_O_NONBLOCK 04000
#define QUASI_O_NDELAY O_NONBLOCK
#define QUASI_O_SYNC 04010000
#define QUASI_O_FSYNC O_SYNC
#define QUASI_O_ASYNC 020000
#define __O_LARGEFILE 0100000

#define __O_DIRECTORY 0200000
#define __O_NOFOLLOW 0400000
#define __O_CLOEXEC 02000000
#define __O_DIRECT 040000
#define __O_NOATIME 01000000
#define __O_PATH 010000000
#define __O_DSYNC 010000
#define __O_TMPFILE (020000000 | __O_DIRECTORY)

#define QUASI_O_LARGEFILE __O_LARGEFILE
#define QUASI_O_DIRECTORY __O_DIRECTORY /* Must be a directory.  */
#define QUASI_O_NOFOLLOW __O_NOFOLLOW   /* Do not follow links.  */
#define QUASI_O_CLOEXEC __O_CLOEXEC     /* Set close_on_exec.  */

#define QUASI_O_DIRECT __O_DIRECT   /* Direct disk access.  */
#define QUASI_O_NOATIME __O_NOATIME /* Do not set atime.  */
#define QUASI_O_PATH __O_PATH       /* Resolve pathname but do not open file.  */
#define QUASI_O_TMPFILE __O_TMPFILE /* Atomically create nameless file.  */

#define QUASI_O_DSYNC __O_DSYNC /* Synchronize data.  */
#define QUASI_O_RSYNC O_SYNC    /* Synchronize read operations.  */
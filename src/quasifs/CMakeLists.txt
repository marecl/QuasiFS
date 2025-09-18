# Host IO
add_library(host_io_lib
    host_io_posix.cpp
    host_io_win32.cpp
    )

target_include_directories(host_io_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# QuasiFS
add_library(quasifs_lib
    quasifs.cpp
    quasifs_inode_directory.cpp
    quasifs_inode_regularfile.cpp
    quasifs_inode_symlink.cpp
    quasifs_partition.cpp    
    )

target_include_directories(quasifs_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
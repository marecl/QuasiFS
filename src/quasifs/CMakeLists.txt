# biblioteka z plików źródłowych
add_library(quasifs_lib
    quasifs.cpp
    quasifs_inode_directory.cpp
    quasifs_inode_regularfile.cpp
    quasifs_inode_symlink.cpp
    quasifs_partition.cpp    
    )

# wskaż katalog z nagłówkami
target_include_directories(quasifs_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
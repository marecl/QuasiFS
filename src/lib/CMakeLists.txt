# biblioteka z plików źródłowych
add_library(quasifs_lib
    quasi_fs.cpp
    quasi_inode_directory.cpp
    quasi_inode_regularfile.cpp
    quasi_partition.cpp    
    )

# wskaż katalog z nagłówkami
target_include_directories(quasifs_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
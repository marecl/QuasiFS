# biblioteka z plików źródłowych
add_library(quasifs_lib quasi_fs.cpp)

# wskaż katalog z nagłówkami
target_include_directories(quasifs_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
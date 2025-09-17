# biblioteka z plików źródłowych
add_library(host_io_lib
    host_io.cpp
    )

# wskaż katalog z nagłówkami
target_include_directories(host_io_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
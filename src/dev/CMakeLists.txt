# biblioteka z plików źródłowych
add_library(dev
    dev_std.cpp
    )

# wskaż katalog z nagłówkami
target_include_directories(dev
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)
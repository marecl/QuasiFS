cmake_minimum_required(VERSION 3.15)

# Nazwa projektu i standard
project(quasi_fs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pliki źródłowe
set(SOURCES
    src/main.cpp
)

add_subdirectory(src/lib)
add_subdirectory(src/dev)

# Tworzymy plik wykonywalny
add_executable(quasi_fs ${SOURCES})

target_link_libraries(quasi_fs PRIVATE quasifs_lib dev)
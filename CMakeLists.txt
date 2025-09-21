cmake_minimum_required(VERSION 3.15)

project(quasi_fs LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


add_subdirectory(src/dev)
add_subdirectory(src/hostio)
add_subdirectory(src/quasifs)

set(SOURCES
    src/main.cpp
)

add_executable(quasi_fs ${SOURCES})

target_link_libraries(quasi_fs PRIVATE dev host_io_lib quasifs_lib)
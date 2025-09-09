cmake_minimum_required(VERSION 3.15)

# Nazwa projektu i standard
project(projekt LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pliki źródłowe
set(SOURCES
    src/main.cpp
)

# Tworzymy plik wykonywalny
add_executable(projekt ${SOURCES})

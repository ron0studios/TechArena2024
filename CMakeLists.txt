cmake_minimum_required(VERSION 3.28)
project(TechArena2024)

set(CMAKE_CXX_STANDARD 17)

add_executable(TechArena2024 main.cpp
        include/TrackOne.cpp
        lib/TrackOne.h)

cmake_minimum_required(VERSION 3.5)
project(revoke)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

#ifeq ($(OS),Darwin)
#LINK_FLAGS :=
    #else
#LINK_FLAGS := -Wl,-Bstatic -static-libgcc -static-libstdc++
    #endif

add_subdirectory(lua)

add_library(revoke SHARED test.cpp)
target_link_libraries(revoke PRIVATE lua)


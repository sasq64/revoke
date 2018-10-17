cmake_minimum_required(VERSION 3.5)
project(lua)

set(CORE_FILES lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c ltm.c lundump.c lvm.c lzio.c)
set(LIB_FILES lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c)
set(LUA_SOURCES ${CORE_FILES} ${LIB_FILES})

add_library(lua ${LUA_SOURCES})
target_include_directories(lua INTERFACE ${PROJECT_SOURCE_DIR})
#target_link_libraries(lua PUBLIC xx)


if(ANDROID)
    target_link_libraries(machlib PRIVATE -lGLESv3 -llog)
# else()
#     add_executable(testmachine test.cpp ../testmain.cpp)
#     target_link_libraries(testmachine PRIVATE machine image)
#     target_compile_definitions(testmachine PRIVATE -DTESTING)
endif()


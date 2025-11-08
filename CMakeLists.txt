cmake_minimum_required(VERSION 3.23)

project(Lattice VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

option(USE_LUAJIT "Use LuaJit" TRUE)
if(USE_LUAJIT)
    find_package(LuaJit REQUIRED)
else()
    find_package(Lua REQUIRED)
endif()

if(NOT TARGET Lua::Lua)
    add_library(Lua::Lua INTERFACE IMPORTED)
    set_target_properties(
        Lua::Lua
        PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
    )
endif()

if (MSVC)
    add_compile_options(/WX /W4)
else()
    add_compile_options(-Werror -Wall)
endif()

add_subdirectory(lattice)
add_subdirectory(tests)

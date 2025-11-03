find_path(LUA_INCLUDE_DIR luajit.h)
find_library(LUA_LIBRARY luajit-5.1 luajit)

if(NOT "${LUA_LIBRARY}" STREQUAL "LUA_LIBRARY-NOTFOUND")
    set(LUA_LIBRARIES "${LUA_LIBRARY}")
elseif(NOT LUA_LIBRARIES)
    message(FATAL_ERROR "Failed to find LUA_LIBRARIES")
endif()

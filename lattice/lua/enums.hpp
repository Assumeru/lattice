#ifndef LATTICE_LUA_ENUMS_H
#define LATTICE_LUA_ENUMS_H

#include <lua.hpp>

namespace lat
{
    enum class LuaType : int
    {
        Nil = LUA_TNIL,
        Number = LUA_TNUMBER,
        Boolean = LUA_TBOOLEAN,
        String = LUA_TSTRING,
        Table = LUA_TTABLE,
        Function = LUA_TFUNCTION,
        UserData = LUA_TUSERDATA,
        Thread = LUA_TTHREAD,
        LightUserData = LUA_TLIGHTUSERDATA,
    };

    enum class LuaStatus : int
    {
        Ok = LUA_OK,
        Yield = LUA_YIELD,
        RuntimeError = LUA_ERRRUN,
        SyntaxError = LUA_ERRSYNTAX,
        MemoryError = LUA_ERRMEM,
        ErrorHandlingError = LUA_ERRERR,
    };
}

#endif

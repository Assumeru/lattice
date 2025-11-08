#ifndef LATTICE_LUA_ENUMS_H
#define LATTICE_LUA_ENUMS_H

#include <lua.hpp>

#include <type_traits>

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
        Ok = 0,
        Yield = LUA_YIELD,
        RuntimeError = LUA_ERRRUN,
        SyntaxError = LUA_ERRSYNTAX,
        MemoryError = LUA_ERRMEM,
        ErrorHandlingError = LUA_ERRERR,
    };

    enum class LuaHookMask : int
    {
        None = 0,
        Call = LUA_MASKCALL,
        Return = LUA_MASKRET,
        Line = LUA_MASKLINE,
        Count = LUA_MASKCOUNT,
    };

    enum class LuaInfo : int
    {
        FillName = 0x1,
        FillSource = 0x10,
        FillCurrentLine = 0x100,
        FillNumUpValues = 0x1000,
        PushFunction = 0x10000,
        PushValidLines = 0x100000,
    };

    template <class T>
    concept BitFlags = std::is_same_v<LuaHookMask, T> || std::is_same_v<LuaInfo, T>;

    template <BitFlags T>
    static bool operator&(T l, T r)
    {
        return static_cast<int>(l) & static_cast<int>(r);
    }

    template <BitFlags T>
    static T operator|(T l, T r)
    {
        return static_cast<T>(static_cast<int>(l) | static_cast<int>(r));
    }
}

#endif

#ifndef LATTICE_LUA_API_H
#define LATTICE_LUA_API_H

#include <lua.hpp>

#include "enums.hpp"

#include <cinttypes>
#include <string_view>

namespace lat
{
    class LuaApi
    {
        lua_State* mState;

        LuaApi(LuaApi&&) = delete;
        LuaApi(const LuaApi&) = delete;

        int gc(int what, int data) { return lua_gc(mState, what, data); }

    public:
        explicit LuaApi(lua_State& state)
            : mState(&state)
        {
        }

        lua_CFunction setPanicFunction(lua_CFunction panicF) const noexcept { return lua_atpanic(mState, panicF); }

        void call(int numArgs, int numResults) { lua_call(mState, numArgs, numResults); }

        bool checkStack(int extra) { return lua_checkstack(mState, extra); }

        void concat(int n) { lua_concat(mState, n); }

        [[nodiscard]] LuaStatus protectedCall(lua_CFunction func, void* userData = nullptr) const noexcept
        {
            return static_cast<LuaStatus>(lua_cpcall(mState, func, userData));
        }

        void createTable(int arraySize = 0, int objectSize = 0) { lua_createtable(mState, arraySize, objectSize); }

        int dumpFunction(lua_Writer writer, void* data) { return lua_dump(mState, writer, data); }

        bool equal(int index1, int index2) { return lua_equal(mState, index1, index2); }

        [[noreturn]] void error() { lua_error(mState); }

        void stopGarbageCollector() { gc(LUA_GCSTOP, 0); }

        void restartGarbageCollector() { gc(LUA_GCRESTART, 0); }

        void runGarbageCollector() { gc(LUA_GCCOLLECT, 0); }

        int getMemoryUseKiB() { return gc(LUA_GCCOUNT, 0); }

        int getMemoryUseRemainderB() { return gc(LUA_GCCOUNTB, 0); }

        bool runGarbageCollectionStep(int size) { return gc(LUA_GCSTEP, size); }

        int setGCPause(int pause) { return gc(LUA_GCSETPAUSE, pause); }

        int setGCStepMultiplier(int mult) { return gc(LUA_GCSETSTEPMUL, mult); }

        lua_Alloc getAllocator(void** userData = nullptr) const noexcept { return lua_getallocf(mState, userData); }

        void pushEnvTable(int index) const noexcept { lua_getfenv(mState, index); }

        void pushTableValue(int index, const char* key) { lua_getfield(mState, index, key); }

        void pushGlobal(const char* name) { lua_getglobal(mState, name); }

        bool pushMetatable(int index) const noexcept { return lua_getmetatable(mState, index); }

        void pushTableValue(int index) { lua_gettable(mState, index); }

        int getStackSize() const noexcept { return lua_gettop(mState); }

        void insert(int index) const noexcept { lua_insert(mState, index); }

        bool isBoolean(int index) const noexcept { return lua_isboolean(mState, index); }

        bool isCFunction(int index) const noexcept { return lua_iscfunction(mState, index); }

        bool isFunction(int index) const noexcept { return lua_isfunction(mState, index); }

        bool isLightUserData(int index) const noexcept { return lua_islightuserdata(mState, index); }

        bool isNil(int index) const noexcept { return lua_isnil(mState, index); }

        bool isNone(int index) const noexcept { return lua_isnone(mState, index); }

        bool isNoneOrNil(int index) const noexcept { return lua_isnoneornil(mState, index); }

        bool isNumber(int index) const noexcept { return lua_isnumber(mState, index); }

        bool isString(int index) const noexcept { return lua_isstring(mState, index); }

        bool isTable(int index) const noexcept { return lua_istable(mState, index); }

        bool isThread(int index) const noexcept { return lua_isthread(mState, index); }

        bool isUserData(int index) const noexcept { return lua_isuserdata(mState, index); }

        bool lessThan(int index1, int index2) { return lua_lessthan(mState, index1, index2); }

        [[nodiscard]] LuaStatus loadFunction(lua_Reader reader, void* data, const char* chunkName) const noexcept
        {
            return static_cast<LuaStatus>(lua_load(mState, reader, data, chunkName));
        }

        void* createUserData(size_t size) { return lua_newuserdata(mState, size); }

        lua_State* createThread() { return lua_newthread(mState); }

        int next(int index) { return lua_next(mState, index); }

        size_t getObjectSize(int index) const noexcept { return lua_objlen(mState, index); }

        [[nodiscard]] LuaStatus protectedCall(
            int numArgs, int numResults = LUA_MULTRET, int errorHandler = 0) const noexcept
        {
            return static_cast<LuaStatus>(lua_pcall(mState, numArgs, numResults, errorHandler));
        }

        void pop(int num) const noexcept { lua_pop(mState, num); }

        void pushBoolean(bool value) const noexcept { lua_pushboolean(mState, value); }

        void pushFunction(lua_CFunction fn, std::uint8_t upValues) { lua_pushcclosure(mState, fn, upValues); }

        void pushFunction(lua_CFunction fn) const noexcept { lua_pushcfunction(mState, fn); }

        void pushInteger(lua_Integer n) const noexcept { lua_pushinteger(mState, n); }

        void pushLightUserData(void* p) const noexcept { lua_pushlightuserdata(mState, p); }

        void pushString(std::string_view string) { lua_pushlstring(mState, string.data(), string.size()); }

        void pushNil() const noexcept { lua_pushnil(mState); }

        void pushNumber(lua_Number n) const noexcept { lua_pushnumber(mState, n); }

        void pushCString(const char* s) { lua_pushstring(mState, s); }

        bool pushThread(const LuaApi& api) const noexcept { return lua_pushthread(api.mState); }

        void pushCopy(int index) const noexcept { lua_pushvalue(mState, index); }

        bool rawEqual(int index1, int index2) const noexcept { return lua_rawequal(mState, index1, index2); }

        void pushRawTableValue(int index) const noexcept { lua_rawget(mState, index); }

        void pushRawTableValue(int index, int arrayIndex) const noexcept { lua_rawgeti(mState, index, arrayIndex); }

        void setRawTableEntry(int index) { lua_rawset(mState, index); }

        void setRawTableValue(int index, int arrayIndex) { lua_rawseti(mState, index, arrayIndex); }

        void remove(int index) const noexcept { lua_remove(mState, index); }

        void replace(int index) const noexcept { lua_replace(mState, index); }

        [[nodiscard]] LuaStatus resumeThread(int numArgs) const noexcept
        {
            return static_cast<LuaStatus>(lua_resume(mState, numArgs));
        }

        void setAllocator(lua_Alloc f, void* userData) const noexcept { lua_setallocf(mState, f, userData); }

        bool setEnvTable(int index) const noexcept { return lua_setfenv(mState, index); }

        void setTableValue(int index, const char* key) { lua_setfield(mState, index, key); }

        void setGlobalValue(const char* name) { lua_setglobal(mState, name); }

        void setMetatable(int index) const noexcept { lua_setmetatable(mState, index); }

        void setTableEntry(int index) { lua_settable(mState, index); }

        void setStackSize(int index) const noexcept { lua_settop(mState, index); }

        LuaStatus status() const noexcept { return static_cast<LuaStatus>(lua_status(mState)); }

        bool asBoolean(int index) const noexcept { return lua_toboolean(mState, index); }

        lua_CFunction asFunction(int index) const noexcept { return lua_tocfunction(mState, index); }

        lua_Integer asInteger(int index) const noexcept { return lua_tointeger(mState, index); }

        std::string_view toString(int index)
        {
            size_t length;
            const char* value = lua_tolstring(mState, index, &length);
            return std::string_view(value, length);
        }

        lua_Number asNumber(int index) const noexcept { return lua_tonumber(mState, index); }

        lua_State* asThread(int index) const noexcept { return lua_tothread(mState, index); }

        void* asUserData(int index) const noexcept { return lua_touserdata(mState, index); }

        LuaType getType(int index) const noexcept { return static_cast<LuaType>(lua_type(mState, index)); }

        void moveValuesTo(const LuaApi& other, int num) const noexcept { lua_xmove(mState, other.mState, num); }

        [[nodiscard]] int yield(int numResults) const noexcept { return lua_yield(mState, numResults); }

        // const char* getupvalue(int funcindex, int n) const noexcept { return lua_getupvalue(mState, funcindex, n); }

        // const char* setupvalue(int funcindex, int n) const noexcept { return lua_setupvalue(mState, funcindex, n); }

        [[noreturn]] void raiseArgumentError(int argPos, const char* extramsg)
        {
            luaL_argerror(mState, argPos, extramsg);
        }

        bool callMetamethod(int obj, const char* key) { return luaL_callmeta(mState, obj, key); }

        void checkAnyFunctionArgument(int argPos) { luaL_checkany(mState, argPos); }

        lua_Integer checkIntegerFunctionArgument(int argPos) { return luaL_checkinteger(mState, argPos); }

        std::string_view checkToStringFunctionArgument(int argPos)
        {
            size_t length;
            const char* value = luaL_checklstring(mState, argPos, &length);
            return std::string_view(value, length);
        }

        lua_Number checkNumberFunctionArgument(int argPos) { return luaL_checknumber(mState, argPos); }

        void checkFunctionArgumentType(int argPos, LuaType type)
        {
            luaL_checktype(mState, argPos, static_cast<int>(type));
        }

        // void* Lcheckudata(int argPos, const char* name) { return luaL_checkudata(mState, argPos, name); }

        // int pushMetatableValue(int obj, const char* key) { return luaL_getmetafield(mState, obj, key); }

        // void pushMetatable(const char* name) const noexcept { luaL_getmetatable(mState, name); }

        [[nodiscard]] LuaStatus loadFunction(std::string_view buffer, const char* name)
        {
            return static_cast<LuaStatus>(luaL_loadbuffer(mState, buffer.data(), buffer.size(), name));
        }

        bool createOrPushMetatable(const char* name) { return luaL_newmetatable(mState, name); }

        lua_Integer checkIntegerFunctionArgument(int argPos, lua_Integer fallback)
        {
            return luaL_optinteger(mState, argPos, fallback);
        }

        std::string_view checkToStringFunctionArgument(int argPos, std::string_view fallback)
        {
            size_t length = fallback.size();
            const char* value = luaL_optlstring(mState, argPos, fallback.data(), &length);
            return std::string_view(value, length);
        }

        lua_Number checkNumberFunctionArgument(int argPos, lua_Number fallback)
        {
            return luaL_optnumber(mState, argPos, fallback);
        }

        int createReferenceIn(int table) { return luaL_ref(mState, table); }

        [[noreturn]] void raiseArgumentTypeError(int argPos, const char* expected)
        {
            luaL_typerror(mState, argPos, expected);
        }

        void removeReferenceFrom(int table, int ref) const noexcept { luaL_unref(mState, table, ref); }

        void pushPositionString(int lvl = 0) { luaL_where(mState, lvl); }
    };
}

#endif

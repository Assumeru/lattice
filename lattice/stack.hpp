#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "functionref.hpp"

#include <cstdint>
#include <string_view>

struct lua_State;

namespace lat
{
    class LuaApi;

    // Non-owning lua_State wrapper
    class Stack
    {
        lua_State* mState;

        Stack(const Stack&) = delete;
        Stack(Stack&&) = delete;

        friend class State;
        friend struct MainStack;

        LuaApi api();
        const LuaApi api() const;

        void call(FunctionRef<void(Stack&)>);

    public:
        explicit Stack(lua_State*);

        void ensure(std::uint16_t extra);

        void collectGarbage();

        int getTop() const;

        void pushFunction(std::string_view lua, const char* name = "");

        // TODO remove
        lua_State* get() { return mState; }
    };
}

#endif

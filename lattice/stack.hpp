#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "functionref.hpp"

#include <cstdint>

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

        LuaApi api();
        const LuaApi api() const;

        void call(FunctionRef<void(Stack&)>);

    public:
        explicit Stack(lua_State*);

        void ensure(std::uint16_t extra);

        void collectGarbage();

        int getTop() const;

        // TODO remove
        lua_State* get() { return mState; }
    };
}

#endif

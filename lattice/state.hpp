#ifndef LATTICE_STATE_H
#define LATTICE_STATE_H

#include "functionref.hpp"

#include <cstddef>
#include <memory>

struct lua_Debug;

namespace lat
{
    template <class UserData>
    using Allocator = void* (*)(UserData*, void*, std::size_t, std::size_t);

    class Stack;
    struct MainStack;
    enum class LuaHookMask : int;

    // Owning lua_State wrapper.
    class State
    {
        std::unique_ptr<MainStack> mState;

    public:
        State();
        State(Allocator<void>, void*);

        template <class UserData>
        State(Allocator<UserData> allocator, UserData* userData)
            : State(reinterpret_cast<Allocator<void>>(allocator), static_cast<void*>(userData))
        {
        }

        ~State();

        void withStack(FunctionRef<void(Stack&)>) const;

        void setDebugHook(FunctionRef<void(Stack&, lua_Debug&)> hook, LuaHookMask mask, int count = 0) const;
        void disableDebugHook() const;
    };
}

#endif

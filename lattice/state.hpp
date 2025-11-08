#ifndef LATTICE_STATE_H
#define LATTICE_STATE_H

#include "functionref.hpp"

#include <cstddef>
#include <memory>

namespace lat
{
    template <class UserData>
    using Allocator = void* (*)(UserData*, void*, std::size_t, std::size_t);

    class Stack;

    // Owning lua_State wrapper.
    class State
    {
        std::unique_ptr<Stack> mStack;

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
    };
}

#endif

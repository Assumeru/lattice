#ifndef LATTICE_STATE_H
#define LATTICE_STATE_H

#include <cstddef>

struct lua_State;

namespace lat
{
    template <class UserData>
    using Allocator = void* (*)(UserData*, void*, std::size_t, std::size_t);

    class State
    {
        lua_State* mState = nullptr;

        State(const State&) = delete;

    public:
        State();
        State(Allocator<void>, void*);
        State(State&&);

        template <class UserData>
        State(Allocator<UserData> allocator, UserData* userData)
            : State(reinterpret_cast<Allocator<void>>(allocator), static_cast<void*>(userData))
        {
        }

        ~State();

        lua_State* get() const { return mState; }
    };
}

#endif

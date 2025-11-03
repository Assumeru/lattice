#include "state.hpp"

#include <lua.hpp>

#include <type_traits>
#include <utility>

namespace lat
{
    static_assert(std::is_same_v<lua_Alloc, Allocator<void>>);

    State::State()
    {
        mState = luaL_newstate();
    }

    State::State(Allocator<void> allocator, void* userData)
    {
        mState = lua_newstate(allocator, userData);
    }

    State::State(State&& state)
    {
        std::swap(mState, state.mState);
    }

    State::~State()
    {
        if (mState)
        {
            lua_close(mState);
            mState = nullptr;
        }
    }
}

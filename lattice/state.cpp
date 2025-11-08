#include "state.hpp"

#include <lua.hpp>

#include <type_traits>
#include <utility>

#include "stack.hpp"

namespace lat
{
    static_assert(std::is_same_v<lua_Alloc, Allocator<void>>);

    State::State()
    {
        mStack = std::make_unique<Stack>(luaL_newstate());
    }

    State::State(Allocator<void> allocator, void* userData)
    {
        mStack = std::make_unique<Stack>(lua_newstate(allocator, userData));
    }

    State::~State()
    {
        if (mStack)
        {
            lua_close(mStack->mState);
            mStack = nullptr;
        }
    }

    void State::withStack(FunctionRef<void(Stack&)> function) const
    {
        return mStack->call(function);
    }
}

#include "state.hpp"

#include <lua.hpp>

#include <optional>
#include <type_traits>
#include <utility>

#include "api/api.hpp"
#include "stack.hpp"

namespace lat
{
    static_assert(std::is_same_v<lua_Alloc, Allocator<void>>);

    struct State::MainStack
    {
        constexpr auto globalName = "lat.Main";

        Stack mStack;
        std::optional<FunctionRef<void(Stack&, lua_Debug&)>> mDebugHook;

        MainStack(lua_State* state)
            : mStack(state)
        {
            LuaApi api = mStack.api();
            api.pushLightUserData(this);
            api.setGlobalValue(globalName);
        }

        void callDebugHook(lua_Debug* activationRecord)
        {
            if (mDebugHook)
                (*mDebugHook)(mStack, *activationRecord);
        }

        ~MainStack()
        {
            lua_close(mStack->mState);
            mStack = nullptr;
        }
    };

    namespace
    {
        void callDebugHook(lua_State* state, lua_Debug* activationRecord)
        {
            LuaApi api(*state);
            api.pushGlobal(State::MainStack::globalName);
            auto main = static_cast<State::MainStack*>(api.asUserData());
            api.pop(1);
            if (main == nullptr)
            {
                api.pushString("invalid state");
                api.error();
            }
            try
            {
                main.callDebugHook(activationRecord);
                return;
            }
            catch (const std::exception& e)
            {
                api.pushCString(e.what());
            }
            catch (...)
            {
                api.pushString("unknown error");
            }
            api.error();
        }
    }

    State::State()
    {
        mState = std::make_unique<MainStack>(luaL_newstate());
    }

    State::State(Allocator<void> allocator, void* userData)
    {
        mState = std::make_unique<MainStack>(lua_newstate(allocator, userData));
    }

    State::~State() = default;

    void State::withStack(FunctionRef<void(Stack&)> function) const
    {
        return mState->mStack.call(function);
    }

    void State::setDebugHook(FunctionRef<void(Stack&, lua_Debug&)> hook, LuaHookMask mask, int count) const
    {
        mState->mDebugHook = hook;
        mState->mStack.api().setDebugHook(callDebugHook, mask, count);
    }

    void State::disableDebugHook() const
    {
        mState->mDebugHook.reset();
        mState->mStack.api().setDebugHook(nullptr, LuaHookMask::None, 0);
    }
}

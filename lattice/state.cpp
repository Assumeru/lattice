#include "state.hpp"

#include <lua.hpp>

#include <optional>
#include <type_traits>
#include <utility>

#include "lua/api.hpp"
#include "stack.hpp"

namespace lat
{
    static_assert(std::is_same_v<lua_Alloc, Allocator<void>>);

    struct MainStack
    {
        static constexpr auto globalName = "lat.Main";

        Stack mStack;
        std::optional<FunctionRef<void(Stack&, lua_Debug&)>> mDebugHook;

        MainStack(lua_State* state)
            : mStack(state)
        {
            mStack.protectedCall(
                [](lua_State* state) {
                    LuaApi(*state).setGlobalValue(globalName);
                    return 0;
                },
                this);
        }

        void callDebugHook(lua_Debug* activationRecord)
        {
            if (mDebugHook)
                (*mDebugHook)(mStack, *activationRecord);
        }

        ~MainStack() { lua_close(mStack.mState); }
    };

    namespace
    {
        void callDebugHook(lua_State* state, lua_Debug* activationRecord)
        {
            LuaApi api(*state);
            api.pushGlobal(MainStack::globalName);
            auto main = static_cast<MainStack*>(api.asUserData(-1));
            api.pop(1);
            if (main == nullptr)
            {
                api.pushString("invalid state");
                api.error();
            }
            try
            {
                main->callDebugHook(activationRecord);
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

    void State::loadLibraries(std::span<const Library> libraries) const
    {
        mState->mStack.protectedCall(
            [](lua_State* state) {
                LuaApi api(*state);
                auto libs = *static_cast<std::span<const Library>*>(api.asUserData(-1));
                api.pop(1);
                if (libs.empty())
                {
                    api.openAllLibraries();
                    return 0;
                }
                auto load = [&](auto f) {
                    int pushed = f(state);
                    if (pushed > 0)
                        api.pop(pushed);
                };
                for (Library lib : libs)
                {
                    switch (lib)
                    {
                        case Library::Base:
                            load(luaopen_base);
                            break;
                        case Library::Package:
                            load(luaopen_package);
                            break;
                        case Library::String:
                            load(luaopen_string);
                            break;
                        case Library::Table:
                            load(luaopen_table);
                            break;
                        case Library::Math:
                            load(luaopen_math);
                            break;
                        case Library::IO:
                            load(luaopen_io);
                            break;
                        case Library::OS:
                            load(luaopen_os);
                            break;
                        case Library::Debug:
                            load(luaopen_debug);
                            break;
#ifdef LAT_LUAJIT
                        case Library::Bit:
                            load(luaopen_bit);
                            break;
                        case Library::JIT:
                            load(luaopen_jit);
                            break;
                        case Library::FFI:
                            load(luaopen_ffi);
                            break;
                        case Library::StringBuffer:
                            load(luaopen_string_buffer);
                            break;
#endif
                        default:
                            break;
                    }
                }
                return 0;
            },
            &libraries);
    }
}

#include "state.hpp"

#include <lua.hpp>

#include <optional>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "lua/api.hpp"
#include "reference.hpp"
#include "stack.hpp"

namespace lat
{
    static_assert(std::is_same_v<lua_Alloc, Allocator<void>>);

    struct MainStack
    {
        static constexpr auto globalName = "lat.Main";

        Stack mStack;
        std::optional<FunctionRef<void(Stack&, lua_Debug&)>> mDebugHook;
        std::unordered_map<std::type_index, TableReference> mMetatables;

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

        ~MainStack()
        {
            mMetatables.clear();
            lua_close(mStack.mState);
        }
    };

    namespace
    {
        MainStack* getMainStack(LuaApi& api)
        {
            api.pushGlobal(MainStack::globalName);
            auto main = static_cast<MainStack*>(api.asUserData(-1));
            api.pop(1);
            return main;
        }

        void callDebugHook(lua_State* state, lua_Debug* activationRecord)
        {
            LuaApi api(*state);
            MainStack* main = getMainStack(api);
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

        using Destructor = void (*)(void*);

        int defaultDestructor(lua_State* state)
        {
            LuaApi api(*state);
            void* data = api.asUserData(-1);
            if (data == nullptr)
            {
                api.pushString("missing userdata parameter");
                api.error();
            }
            constexpr std::size_t pointerSize = sizeof(void*);
            std::size_t size = api.getObjectSize(-1);
            if (size < pointerSize)
            {
                api.pushString("invalid userdata");
                api.error();
            }
            else if (size == pointerSize)
            {
                // "light" userdata; pointer only
                return 0;
            }
            void* pointer = nullptr;
            std::memcpy(&pointer, data, pointerSize);
            if (pointer == nullptr)
            {
                // nothing to destroy
                return 0;
            }
            api.pushUpValue(1);
            Destructor destructor = static_cast<Destructor>(api.asUserData(-1));
            if (destructor == nullptr)
            {
                api.pushString("missing destructor");
                api.error();
            }
            try
            {
                destructor(pointer);
                pointer = nullptr;
                std::memcpy(data, &pointer, pointerSize);
                return 0;
            }
            catch (const std::exception& e)
            {
                api.pushString(e.what());
            }
            catch (...)
            {
                api.pushString("error in destructor");
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

    Stack& State::getMain(BasicStack& stack)
    {
        stack.ensure(1);
        LuaApi api = stack.api();
        MainStack* main = getMainStack(api);
        return main->mStack;
    }

    const TableReference& State::getMetatable(BasicStack& stack, const std::type_index& type, Destructor destructor)
    {
        stack.ensure(1);
        LuaApi api = stack.api();
        MainStack* main = getMainStack(api);
        auto found = main->mMetatables.find(type);
        if (found == main->mMetatables.end())
        {
            TableView table = stack.pushTable();
            stack.ensure(1);
            api.pushLightUserData(destructor);
            api.pushFunction(&defaultDestructor, 1);
            table[meta::gc] = stack.getObject(-1);
            found = main->mMetatables.emplace(type, table.store()).first;
        }
        return found->second;
    }

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

#include "stack.hpp"

#include <lua.hpp>

#include <new>
#include <stdexcept>

#ifdef LUAI_MAXCSTACK
#include <limits>
static_assert(std::numeric_limits<std::uint16_t>::max() >= LUAI_MAXCSTACK);
#endif

#include "lua/api.hpp"

#include <iostream>

namespace
{
    constexpr std::uint16_t allocationOverhead = 3;
    constexpr std::uint16_t initialSize = LUA_MINSTACK - allocationOverhead;

    [[noreturn]] void throwLuaError(lat::LuaApi& lua, const char* fallback)
    {
        if (!lua.isNone(-1) && lua.isString(-1))
        {
            // Per the spec this function can throw to support converting non-string values to string. Neither
            // Lua nor LuaJIT throw if the value is already a string, making this safe outside of a protected
            // call.
            std::string_view message = lua.toString(-1);
            // Lua strings are 0-terminated making this okay.
            std::runtime_error error(message.data());
            lua.pop(1);
            throw error;
        }
        throw std::runtime_error(fallback);
    }
}

namespace lat
{
    Stack::Stack(lua_State* state)
        : mState(state)
    {
        if (state == nullptr)
            throw std::bad_alloc();
    }

    LuaApi Stack::api()
    {
        return LuaApi(*mState);
    }

    const LuaApi Stack::api() const
    {
        return LuaApi(*mState);
    }

    void Stack::call(FunctionRef<void(Stack&)> function)
    {
        LuaApi lua = api();
        LuaStatus status = lua.protectedCall(
            [](lua_State* state) -> int {
                LuaApi api(*state);
                auto consumer = static_cast<FunctionRef<void(Stack&)>*>(api.asUserData(-1));
                api.pop(1);
                try
                {
                    Stack s(state);
                    (*consumer)(s);
                    return 0;
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
            },
            &function);
        switch (status)
        {
            case LuaStatus::ErrorHandlingError:
                throwLuaError(lua, "Lua error handler failed");
            case LuaStatus::MemoryError:
                throwLuaError(lua, "Lua out of memory");
            case LuaStatus::RuntimeError:
                throwLuaError(lua, "Lua error");
            case LuaStatus::Ok:
                return;
            default:
                throw std::logic_error("invalid Lua state");
        }
    }

    void Stack::ensure(std::uint16_t extra)
    {
        if (!api().checkStack(extra))
            throw std::runtime_error("exceeded maximum stack size");
    }

    void Stack::collectGarbage()
    {
        api().runGarbageCollector();
    }

    int Stack::getTop() const
    {
        return api().getStackSize();
    }
}

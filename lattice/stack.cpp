#include "stack.hpp"

#include <lua.hpp>

#include <new>
#include <stdexcept>

#include "lua/api.hpp"
#include "object.hpp"

namespace
{
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

    void Stack::protectedCall(lua_CFunction function, void* userData)
    {
        LuaApi lua = api();
        LuaStatus status = lua.protectedCall(function, userData);
        switch (status)
        {
            case LuaStatus::ErrorHandlingError:
                throwLuaError(lua, "error handler failed");
            case LuaStatus::MemoryError:
                throwLuaError(lua, "out of memory");
            case LuaStatus::RuntimeError:
                throwLuaError(lua, "error");
            case LuaStatus::Ok:
                return;
            default:
                throw std::logic_error("invalid state");
        }
    }

    void Stack::call(FunctionRef<void(Stack&)> function)
    {
        protectedCall(
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
    }

    TableView Stack::globals()
    {
        return TableView(*this, LUA_GLOBALSINDEX);
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

    int Stack::makeAbsolute(int index) const
    {
        if (index <= LUA_REGISTRYINDEX)
        {
            if (index < LUA_GLOBALSINDEX)
                throw std::out_of_range("invalid index");
        }
        else if (index < 0)
            return getTop() + index + 1;
        return index;
    }

    int Stack::getTop() const
    {
        return api().getStackSize();
    }

    void Stack::pop(std::uint16_t amount)
    {
        if (amount > 0)
        {
            LuaApi lua = api();
            if (amount > lua.getStackSize())
                throw std::invalid_argument("cannot pop more than stack size");
            lua.pop(amount);
        }
    }

    bool Stack::isBoolean(int index) const
    {
        return api().isBoolean(index);
    }

    bool Stack::isNil(int index) const
    {
        return api().isNil(index);
    }

    bool Stack::isNumber(int index) const
    {
        return api().isNumber(index);
    }

    bool Stack::isString(int index) const
    {
        return api().isString(index);
    }

    bool Stack::isTable(int index) const
    {
        return api().isTable(index);
    }

    ObjectView Stack::getObject(int index)
    {
        return tryGetObject(index).value();
    }

    std::optional<ObjectView> Stack::tryGetObject(int index)
    {
        if (api().isNone(index))
            return {};
        return ObjectView(*this, makeAbsolute(index));
    }

    void Stack::pushFunction(std::string_view script, const char* name)
    {
        if (script.starts_with(LUA_SIGNATURE[0]))
            throw std::invalid_argument("argument cannot be bytecode");
        ensure(1);
        LuaApi lua = api();
        LuaStatus status = lua.loadFunction(script, name);
        switch (status)
        {
            case LuaStatus::SyntaxError:
                throwLuaError(lua, "syntax error");
            case LuaStatus::MemoryError:
                throwLuaError(lua, "out of memory");
            case LuaStatus::Ok:
                return;
            default:
                throw std::logic_error("invalid state");
        }
    }
}

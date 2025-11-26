#include "stack.hpp"

#include <lua.hpp>

#include <algorithm>
#include <new>
#include <stdexcept>

#include "function.hpp"
#include "lua/api.hpp"
#include "object.hpp"
#include "reference.hpp"
#include "state.hpp"
#include "userdata.hpp"

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

    void checkProtectedCallStatus(lat::LuaApi& lua, lat::LuaStatus status)
    {
        switch (status)
        {
            case lat::LuaStatus::ErrorHandlingError:
                throwLuaError(lua, "error handler failed");
            case lat::LuaStatus::MemoryError:
                throwLuaError(lua, "out of memory");
            case lat::LuaStatus::RuntimeError:
                throwLuaError(lua, "error");
            case lat::LuaStatus::Ok:
                return;
            default:
                throw std::logic_error("invalid state");
        }
    }

    void ensure(auto&& api, std::uint16_t extra)
    {
        if (!api.checkStack(extra))
            throw std::runtime_error("exceeded maximum stack size");
    }

    int push(lat::LuaApi api, auto method, auto... args)
    {
        ensure(api, 1);
        (api.*method)(args...);
        return api.getStackSize();
    }

    int invokeFunction(lua_State* state)
    {
        lat::LuaApi api(*state);
        try
        {
            lat::Stack stack(state);
            api.pushUpValue(1);
            auto function = stack.getObject(-1).as<std::function<int(lat::Stack&)>*>();
            api.pop(1);
            return (*function)(stack);
        }
        catch (const std::exception& e)
        {
            api.setStackSize(0);
            api.pushCString(e.what());
        }
        catch (...)
        {
            api.setStackSize(0);
            api.pushString("unknown error");
        }
        api.error();
    }
}

namespace lat
{
    BasicStack::BasicStack(lua_State* state)
        : mState(state)
    {
        if (state == nullptr)
            throw std::bad_alloc();
    }

    Stack::Stack(lua_State* state)
        : BasicStack(state)
    {
    }

    LuaApi BasicStack::api()
    {
        return LuaApi(*mState);
    }

    const LuaApi BasicStack::api() const
    {
        return LuaApi(*mState);
    }

    void BasicStack::protectedCall(lua_CFunction function, void* userData)
    {
        LuaApi lua = api();
        LuaStatus status = lua.protectedCall(function, userData);
        checkProtectedCallStatus(lua, status);
    }

    void BasicStack::protectedCall(int argCount, int resCount)
    {
        LuaApi lua = api();
        LuaStatus status = lua.protectedCall(argCount, resCount);
        checkProtectedCallStatus(lua, status);
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
                    api.setStackSize(0);
                    api.pushCString(e.what());
                }
                catch (...)
                {
                    api.setStackSize(0);
                    api.pushString("unknown error");
                }
                api.error();
            },
            &function);
    }

    Reference BasicStack::store(int index)
    {
        Stack& main = State::getMain(*this);
        LuaApi lua = api();
        ::ensure(lua, 1);
        lua.pushCopy(index);
        int ref = lua.createReferenceIn(LUA_REGISTRYINDEX);
        return Reference(*main.mState, ref);
    }

    TableView BasicStack::globals()
    {
        return TableView(*this, LUA_GLOBALSINDEX);
    }

    void BasicStack::ensure(std::uint16_t extra)
    {
        ::ensure(api(), extra);
    }

    void BasicStack::collectGarbage()
    {
        api().runGarbageCollector();
    }

    bool BasicStack::collectGarbage(int size)
    {
        return api().runGarbageCollectionStep(size);
    }

    int BasicStack::makeAbsolute(int index) const
    {
        if (index <= LUA_REGISTRYINDEX)
        {
            if (index < LUA_GLOBALSINDEX)
                throw std::out_of_range("invalid index");
        }
        else if (index < 0)
        {
            index = getTop() + index + 1;
            if (index <= 0)
                throw std::out_of_range("invalid index");
        }
        return index;
    }

    int BasicStack::getTop() const
    {
        return api().getStackSize();
    }

    void BasicStack::pop(std::uint16_t amount)
    {
        if (amount > 0)
        {
            LuaApi lua = api();
            if (amount > lua.getStackSize())
                throw std::invalid_argument("cannot pop more than stack size");
            lua.pop(amount);
        }
    }

    void BasicStack::remove(int index)
    {
        LuaApi lua = api();
        if (index < 0)
            index = makeAbsolute(index);
        if (index <= 0 || index > lua.getStackSize())
            throw std::out_of_range("invalid index");
        lua.remove(index);
    }

    bool BasicStack::isBoolean(int index) const
    {
        return api().isBoolean(index);
    }

    bool BasicStack::isNil(int index) const
    {
        return api().isNil(index);
    }

    bool BasicStack::isNumber(int index) const
    {
        return api().isNumber(index);
    }

    bool BasicStack::isString(int index) const
    {
        return api().isString(index);
    }

    bool BasicStack::isTable(int index) const
    {
        return api().isTable(index);
    }

    bool BasicStack::isFunction(int index) const
    {
        return api().isFunction(index);
    }

    bool BasicStack::isCoroutine(int index) const
    {
        return api().isThread(index);
    }

    bool BasicStack::isUserData(int index) const
    {
        return api().isUserData(index);
    }

    bool BasicStack::isLightUserData(int index) const
    {
        return api().isLightUserData(index);
    }

    ObjectView BasicStack::getObject(int index)
    {
        return tryGetObject(index).value();
    }

    std::optional<ObjectView> BasicStack::tryGetObject(int index)
    {
        if (api().isNone(index))
            return {};
        return ObjectView(*this, makeAbsolute(index));
    }

    ObjectView BasicStack::pushNil()
    {
        return ObjectView(*this, push(api(), &LuaApi::pushNil));
    }

    ObjectView BasicStack::pushBoolean(bool value)
    {
        return ObjectView(*this, push(api(), &LuaApi::pushBoolean, value));
    }

    ObjectView BasicStack::pushInteger(std::ptrdiff_t value)
    {
        return ObjectView(*this, push(api(), &LuaApi::pushInteger, value));
    }

    ObjectView BasicStack::pushNumber(double value)
    {
        return ObjectView(*this, push(api(), &LuaApi::pushNumber, value));
    }

    ObjectView BasicStack::pushString(std::string_view value)
    {
        return ObjectView(*this, push(api(), &LuaApi::pushString, value));
    }

    TableView BasicStack::pushTable(int objectSize, int arraySize)
    {
        return TableView(*this, push(api(), &LuaApi::createTable, std::max(arraySize, 0), std::max(objectSize, 0)));
    }

    FunctionView BasicStack::pushFunction(std::string_view script, const char* name)
    {
        if (script.starts_with(LUA_SIGNATURE[0]))
            throw std::invalid_argument("argument cannot be bytecode");
        LuaApi lua = api();
        ::ensure(lua, 1);
        if (name == nullptr)
            name = "";
        LuaStatus status = lua.loadFunction(script, name);
        switch (status)
        {
            case LuaStatus::SyntaxError:
                throwLuaError(lua, "syntax error");
            case LuaStatus::MemoryError:
                throwLuaError(lua, "out of memory");
            case LuaStatus::Ok:
                return FunctionView(*this, lua.getStackSize());
            default:
                throw std::logic_error("invalid state");
        }
    }

    FunctionView BasicStack::pushFunctionImpl(std::function<int(Stack&)> function)
    {
        State::getUserTypeRegistry(*this).pushValue(*this, std::move(function));
        api().pushFunction(&invokeFunction, 1);
        return getObject(-1).asFunction();
    }

    ObjectView BasicStack::pushLightUserData(void* value)
    {
        return ObjectView(*this, push(api(), &LuaApi::pushLightUserData, value));
    }

    std::span<std::byte> BasicStack::pushUserData(std::size_t size)
    {
        LuaApi lua = api();
        ::ensure(lua, 1);
        void* data = lua.createUserData(size);
        return { reinterpret_cast<std::byte*>(data), size };
    }
}

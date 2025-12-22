#include "stack.hpp"

#include <lua.hpp>

#include <algorithm>
#include <new>
#include <stdexcept>

#include "exception.hpp"
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
        if (lua.getType(-1) == lat::LuaType::String)
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

    [[noreturn]] void raiseLuaError(lat::LuaApi& api, std::string_view message)
    {
        api.setStackSize(0);
        api.pushPositionString(1);
        std::string_view position = api.toString(-1);
        api.pop(1);
        if (position.empty())
            api.pushString(message);
        else
        {
            std::string error(position);
            error += message;
            api.pushString(error);
        }
        api.error();
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
        catch (const lat::ArgumentTypeError& e)
        {
            // Free up what space we can, but keep the bad index so the error message can show its type
            if (api.getStackSize() > e.getIndex())
                api.setStackSize(e.getIndex());
            api.raiseArgumentTypeError(e.getIndex(), e.getType().data());
        }
        catch (const std::exception& e)
        {
            raiseLuaError(api, e.what());
        }
        catch (...)
        {
            raiseLuaError(api, "unknown error");
        }
    }

    int loadFunction(lat::LuaApi lua, std::string_view script, const char* name)
    {
        ensure(lua, 1);
        if (name == nullptr)
            name = "";
        lat::LuaStatus status = lua.loadFunction(script, name);
        switch (status)
        {
            case lat::LuaStatus::SyntaxError:
                throwLuaError(lua, "syntax error");
            case lat::LuaStatus::MemoryError:
                throwLuaError(lua, "out of memory");
            case lat::LuaStatus::Ok:
                return lua.getStackSize();
            default:
                throw std::logic_error("invalid state");
        }
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

    LuaApi Stack::api() const
    {
        return LuaApi(*mState);
    }

    void Stack::protectedCall(lua_CFunction function, void* userData)
    {
        LuaApi lua = api();
        LuaStatus status = lua.protectedCall(function, userData);
        checkProtectedCallStatus(lua, status);
    }

    void Stack::protectedCall(int argCount, int resCount)
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
                    raiseLuaError(api, e.what());
                }
                catch (...)
                {
                    raiseLuaError(api, "unknown error");
                }
            },
            &function);
    }

    Reference Stack::store(int index)
    {
        Stack& main = State::getMain(*this);
        LuaApi lua = api();
        ::ensure(lua, 1);
        lua.pushCopy(index);
        int ref = lua.createReferenceIn(LUA_REGISTRYINDEX);
        return Reference(*main.mState, ref);
    }

    TableView Stack::globals()
    {
        return TableView(*this, LUA_GLOBALSINDEX);
    }

    void Stack::ensure(std::uint16_t extra)
    {
        ::ensure(api(), extra);
    }

    void Stack::collectGarbage()
    {
        api().runGarbageCollector();
    }

    bool Stack::collectGarbage(int size)
    {
        return api().runGarbageCollectionStep(size);
    }

    void* Stack::getAllocatorData() const
    {
        void* data;
        api().getAllocator(&data);
        return data;
    }

    int Stack::makeAbsolute(int index) const
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

    void Stack::remove(int index)
    {
        LuaApi lua = api();
        if (index < 0)
            index = makeAbsolute(index);
        if (index <= 0 || index > lua.getStackSize())
            throw std::out_of_range("invalid index");
        lua.remove(index);
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
        return api().getType(index) == LuaType::Number;
    }

    bool Stack::isString(int index) const
    {
        return api().getType(index) == LuaType::String;
    }

    bool Stack::isTable(int index) const
    {
        return api().isTable(index);
    }

    bool Stack::isTableLike(int index)
    {
        LuaApi lua = api();
        if (lua.isTable(index))
            return true;
        ::ensure(lua, 2);
        if (!lua.pushMetatable(index))
            return false;
        lua.pushString(meta::index);
        lua.pushTableValue(-2);
        const bool hasMetaIndex = lua.isFunction(-1);
        lua.pop(2);
        return hasMetaIndex;
    }

    bool Stack::isFunction(int index) const
    {
        return api().isFunction(index);
    }

    bool Stack::isCoroutine(int index) const
    {
        return api().isThread(index);
    }

    bool Stack::isUserData(int index) const
    {
        return api().isUserData(index);
    }

    bool Stack::isLightUserData(int index) const
    {
        return api().isLightUserData(index);
    }

    ObjectView Stack::getObject(int index)
    {
        std::optional<ObjectView> object = tryGetObject(index);
        if (object)
            return *object;
        throw TypeError("object");
    }

    std::optional<ObjectView> Stack::tryGetObject(int index)
    {
        if (api().isNone(index))
            return {};
        return ObjectView(*this, makeAbsolute(index));
    }

    ObjectView Stack::pushNil()
    {
        return ObjectView(*this, ::push(api(), &LuaApi::pushNil));
    }

    ObjectView Stack::pushBoolean(bool value)
    {
        return ObjectView(*this, ::push(api(), &LuaApi::pushBoolean, value));
    }

    ObjectView Stack::pushInteger(std::ptrdiff_t value)
    {
        return ObjectView(*this, ::push(api(), &LuaApi::pushInteger, value));
    }

    ObjectView Stack::pushNumber(double value)
    {
        return ObjectView(*this, ::push(api(), &LuaApi::pushNumber, value));
    }

    ObjectView Stack::pushString(std::string_view value)
    {
        return ObjectView(*this, ::push(api(), &LuaApi::pushString, value));
    }

    TableView Stack::pushTable(int objectSize, int arraySize)
    {
        return TableView(*this, ::push(api(), &LuaApi::createTable, std::max(arraySize, 0), std::max(objectSize, 0)));
    }

    TableView Stack::pushArray(int size)
    {
        return pushTable(0, size);
    }

    FunctionView Stack::pushFunction(std::string_view script, const char* name)
    {
        if (script.starts_with(LUA_SIGNATURE[0]))
            throw std::invalid_argument("argument cannot be bytecode");
        return FunctionView(*this, loadFunction(api(), script, name));
    }

    FunctionView Stack::pushFunction(const ByteCode& code, const char* name)
    {
        return FunctionView(*this, loadFunction(api(), code.get(), name));
    }

    FunctionView Stack::pushFunctionImpl(std::function<int(Stack&)> function)
    {
        State::getUserTypeRegistry(*this).pushValue(*this, std::move(function));
        api().pushFunction(&invokeFunction, 1);
        return getObject(-1).asFunction();
    }

    ObjectView Stack::pushLightUserData(void* value)
    {
        return ObjectView(*this, ::push(api(), &LuaApi::pushLightUserData, value));
    }

    std::span<std::byte> Stack::pushUserData(std::size_t size)
    {
        LuaApi lua = api();
        ::ensure(lua, 1);
        void* data = lua.createUserData(size);
        return { reinterpret_cast<std::byte*>(data), size };
    }
}

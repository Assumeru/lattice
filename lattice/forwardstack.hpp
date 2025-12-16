#ifndef LATTICE_FORWARDSTACK_H
#define LATTICE_FORWARDSTACK_H

#include "functionref.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

struct lua_State;

namespace lat
{
    class ByteCode;
    class FunctionView;
    class LuaApi;
    class ObjectView;
    class Reference;
    class TableView;
    class UserType;

    // Non-owning lua_State wrapper
    class Stack
    {
    protected:
        lua_State* mState;

        Stack(const Stack&) = delete;
        Stack(Stack&&) = delete;

        LuaApi api();
        const LuaApi api() const;

        void protectedCall(int (*)(lua_State*), void*);
        void protectedCall(int, int);
        void call(FunctionRef<void(Stack&)>);

        Reference store(int);

        FunctionView pushFunctionImpl(std::function<int(Stack&)>);

        friend class FunctionView;
        friend struct MainStack;
        friend class ObjectView;
        friend class Reference;
        friend class State;
        friend class TableView;
        friend class UserTypeRegistry;

    public:
        explicit Stack(lua_State*);

        TableView globals();
        auto operator[](auto key);

        void ensure(std::uint16_t extra);

        void collectGarbage();
        bool collectGarbage(int size);

        int makeAbsolute(int index) const;
        int getTop() const;

        void pop(std::uint16_t amount = 1);
        void remove(int index);

        bool isNil(int index) const;
        bool isBoolean(int index) const;
        bool isNumber(int index) const;
        bool isString(int index) const;
        bool isTable(int index) const;
        bool isFunction(int index) const;
        bool isCoroutine(int index) const;
        bool isUserData(int index) const;
        bool isLightUserData(int index) const;

        ObjectView getObject(int index);
        std::optional<ObjectView> tryGetObject(int index);

        ObjectView pushNil();
        ObjectView pushBoolean(bool);
        ObjectView pushInteger(std::ptrdiff_t);
        ObjectView pushNumber(double);
        ObjectView pushString(std::string_view);
        TableView pushTable(int objectSize = 0, int arraySize = 0);
        TableView pushArray(int size);
        FunctionView pushFunction(std::string_view lua, const char* name = nullptr);
        template <class... Ret>
        auto pushFunctionReturning(std::string_view lua, const char* name = nullptr);
        FunctionView pushFunction(const ByteCode& code, const char* name = nullptr);
        template <class... Ret>
        auto pushFunctionReturning(const ByteCode& code, const char* name = nullptr);
        template <class T>
        FunctionView pushFunction(T&&);
        ObjectView pushLightUserData(void*);
        std::span<std::byte> pushUserData(std::size_t);
        template <class T>
        ObjectView push(T&& value);

        template <class... Ret>
        auto execute(std::string_view lua, const char* name = nullptr);

        template <class T, class... Bases>
        UserType newUserType(std::string_view name);
    };
}

#endif

#ifndef LATTICE_BASICSTACK_H
#define LATTICE_BASICSTACK_H

#include "functionref.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

struct lua_State;

namespace lat
{
    class FunctionView;
    class LuaApi;
    class ObjectView;
    class Reference;
    class Stack;
    class TableView;

    // Non-owning lua_State wrapper
    class BasicStack
    {
    protected:
        lua_State* mState;

        BasicStack(const BasicStack&) = delete;
        BasicStack(BasicStack&&) = delete;
        explicit BasicStack(lua_State*);

        LuaApi api();
        const LuaApi api() const;

        void protectedCall(int (*)(lua_State*), void*);
        void protectedCall(int, int);

        Reference store(int);

        FunctionView pushFunctionImpl(std::function<int(Stack&)>);

        friend class FunctionView;
        friend class ObjectView;
        friend class State;
        friend class TableView;
        friend class UserTypeRegistry;

    public:
        TableView globals();

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
        FunctionView pushFunction(std::string_view lua, const char* name = nullptr);
        ObjectView pushLightUserData(void*);
        std::span<std::byte> pushUserData(std::size_t);
        template <class T>
        FunctionView pushFunction(T&&);
    };
}

#endif

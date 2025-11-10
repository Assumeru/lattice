#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "functionref.hpp"
#include "table.hpp"

#include <cstdint>
#include <optional>
#include <string_view>

struct lua_State;

namespace lat
{
    class LuaApi;
    class ObjectView;
    class TableView;

    // Non-owning lua_State wrapper
    class Stack
    {
        lua_State* mState;

        Stack(const Stack&) = delete;
        Stack(Stack&&) = delete;

        friend class State;
        friend struct MainStack;
        friend class ObjectView;
        friend class IndexedTableView;

        LuaApi api();
        const LuaApi api() const;

        void call(FunctionRef<void(Stack&)>);
        void protectedCall(int (*)(lua_State*), void*);

    public:
        explicit Stack(lua_State*);

        bool operator==(const Stack& other) const { return mState == other.mState; }

        TableView globals();
        IndexedTableView operator[](auto key) { return globals()[key]; }

        void ensure(std::uint16_t extra);

        void collectGarbage();

        int makeAbsolute(int index) const;
        int getTop() const;

        void pop(std::uint16_t amount = 1);

        bool isBoolean(int index) const;
        bool isNil(int index) const;
        bool isNumber(int index) const;
        bool isString(int index) const;
        bool isTable(int index) const;

        ObjectView getObject(int index);
        std::optional<ObjectView> tryGetObject(int index);

        void pushFunction(std::string_view lua, const char* name = "");

        // TODO remove
        lua_State* get() { return mState; }
    };
}

#endif

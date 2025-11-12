#ifndef LATTICE_BASICSTACK_H
#define LATTICE_BASICSTACK_H

#include "functionref.hpp"

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

        friend class TableView;
        friend class ObjectView;

    public:
        bool operator==(const BasicStack& other) const { return mState == other.mState; }

        TableView globals();

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

        ObjectView pushNil();
        ObjectView pushBoolean(bool);
        ObjectView pushInteger(std::ptrdiff_t);
        ObjectView pushNumber(double);
        ObjectView pushString(std::string_view);
        void pushFunction(std::string_view lua, const char* name = "");
    };
}

#endif

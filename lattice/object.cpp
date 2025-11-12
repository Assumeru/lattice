#include "object.hpp"

#include "basicstack.hpp"
#include "lua/api.hpp"
#include "table.hpp"

#include <stdexcept>

namespace lat
{
    bool ObjectView::isNil() const
    {
        return mStack.isNil(mIndex);
    }

    bool ObjectView::isBoolean() const
    {
        return mStack.isBoolean(mIndex);
    }

    bool ObjectView::isNumber() const
    {
        return mStack.isNumber(mIndex);
    }

    bool ObjectView::isString() const
    {
        return mStack.isString(mIndex);
    }

    bool ObjectView::isTable() const
    {
        return mStack.isTable(mIndex);
    }

    LuaType ObjectView::getType() const
    {
        return mStack.api().getType(mIndex);
    }

    bool ObjectView::asBool() const
    {
        LuaApi api = mStack.api();
        bool value = api.asBoolean(mIndex);
        if (!value && !api.isBoolean(mIndex))
            throw std::runtime_error("value is not a boolean");
        return value;
    }

    lua_Integer ObjectView::asInt() const
    {
        LuaApi api = mStack.api();
        lua_Integer value = api.asInteger(mIndex);
        if (value == 0 && !api.isNumber(mIndex))
            throw std::runtime_error("value is not an integer");
        return value;
    }

    lua_Number ObjectView::asFloat() const
    {
        LuaApi api = mStack.api();
        lua_Number value = api.asNumber(mIndex);
        if (value == 0. && !api.isNumber(mIndex))
            throw std::runtime_error("value is not a number");
        return value;
    }

    std::string_view ObjectView::asString() const
    {
        LuaApi api = mStack.api();
        if (api.isString(mIndex))
            return api.toString(mIndex);
        throw std::runtime_error("value is not a string");
    }

    TableView ObjectView::asTable() const
    {
        if (!isTable())
            throw std::runtime_error("value is not a table");
        return TableView(mStack, mIndex);
    }

    ObjectView ObjectView::pushTo(BasicStack& stack)
    {
        const bool sameStack = stack == mStack;
        LuaApi api = mStack.api();
        mStack.ensure(1);
        if (!sameStack)
            stack.ensure(1);
        api.pushCopy(mIndex);
        if (!sameStack)
            api.moveValuesTo(stack.api(), 1);
        return ObjectView(stack, stack.getTop());
    }
}

#include "object.hpp"

#include "basicstack.hpp"
#include "function.hpp"
#include "lua/api.hpp"
#include "table.hpp"

#include <ostream>
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

    bool ObjectView::isFunction() const
    {
        return mStack.isFunction(mIndex);
    }

    LuaType ObjectView::getType() const
    {
        return mStack.api().getType(mIndex);
    }

    Nil ObjectView::asNil() const
    {
        if (!isNil())
            throw std::runtime_error("value is not nil");
        return nil;
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

    FunctionView ObjectView::asFunction() const
    {
        if (!isFunction())
            throw std::runtime_error("value is not a function");
        return FunctionView(mStack, mIndex);
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

    std::ostream& operator<<(std::ostream& stream, ObjectView value)
    {
        if (value.isNil())
            return stream << "nil";
        else if (value.isBoolean())
        {
            if (value.asBool())
                return stream << "true";
            return stream << "false";
        }
        else if (value.isNumber())
            return stream << value.asFloat();
        else if (value.isString())
            return stream << '"' << value.asString() << '"';
        else if (value.isFunction())
            return stream << "<function>";
        else if (value.isTable())
            return stream << "<table>";
        return stream << "invalid object view";
    }
}

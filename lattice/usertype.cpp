#include "usertype.hpp"

#include "function.hpp"
#include "table.hpp"

namespace lat
{
    namespace
    {
        constexpr static std::string_view getKey = "lat.get";
        constexpr static std::string_view setKey = "lat.set";
        constexpr static std::string_view propsKey = "lat.props";
        constexpr static std::string_view indexKey = "lat.index";

        bool isMetaKey(std::string_view key)
        {
            if (!key.starts_with("__"))
                return false;
            if (key == meta::add)
                return true;
            if (key == meta::sub)
                return true;
            if (key == meta::mul)
                return true;
            if (key == meta::div)
                return true;
            if (key == meta::mod)
                return true;
            if (key == meta::pow)
                return true;
            if (key == meta::unm)
                return true;
            if (key == meta::concat)
                return true;
            if (key == meta::len)
                return true;
            if (key == meta::eq)
                return true;
            if (key == meta::lt)
                return true;
            if (key == meta::le)
                return true;
            if (key == meta::call)
                return true;
            if (key == meta::gc)
                return true;
            if (key == meta::mode)
                return true;
            if (key == meta::metatable)
                return true;
            if (key == meta::toString)
                return true;
            return false;
        }

        template <bool swap = false>
        TableView getTable(Stack& stack, const TableReference& metatable, std::string_view key)
        {
            TableView mt = metatable.pushTo(stack);
            if constexpr (swap)
                mt[meta::index] = mt[indexKey];
            mt.get(key);
            stack.remove(-2);
            return stack.getObject(-1).asTable();
        }
    }

    UserType::UserType(Stack& stack, const TableReference& metatable, const FunctionReference& defaultIndex,
        const FunctionReference& defaultNewIndex)
        : mStack(stack)
        , mMetatable(metatable)
        , mDefaultIndex(defaultIndex)
        , mDefaultNewIndex(defaultNewIndex)
    {
        FunctionView init = mStack.pushFunction(R"(
            local mt, getKey, setKey, propsKey, indexKey, defaultNewIndex = ...
            local getters = {}
            mt[getKey] = getters
            local setters = { __newindex = defaultNewIndex }
            mt[setKey] = setters
            local props = {}
            mt[propsKey] = props
            mt[indexKey] = function(t, k)
                local get = getters[k]
                if get ~= nil then
                    return get(t)
                end
                get = getters.__index
                if get ~= nil then
                    return get(t, k)
                end
                return props[k]
            end
            mt.__index = props
            mt.__newindex = function(t, k, v)
                local set = setters[k]
                if set ~= nil then
                    set(t, v)
                else
                    set = setters.__newindex
                    set(t, k, v)
                end
            end
            )",
            "latticeUserTypeInit");
        init(mMetatable, getKey, setKey, propsKey, indexKey, mDefaultNewIndex);
        mStack.pop();
    }

    IndexedUserType UserType::operator[](std::string_view key)
    {
        return IndexedUserType(*this, key);
    }

    TableView UserType::props() const
    {
        return getTable(mStack, mMetatable, propsKey);
    }

    TableView UserType::getters() const
    {
        return getTable<true>(mStack, mMetatable, getKey);
    }

    TableView UserType::setters() const
    {
        return getTable(mStack, mMetatable, setKey);
    }

    TableView UserType::props(std::string_view key) const
    {
        if (key == meta::index)
            return getters();
        else if (key == meta::newIndex)
            return setters();
        else if (isMetaKey(key))
            return mMetatable.pushTo(mStack);
        return props();
    }
}

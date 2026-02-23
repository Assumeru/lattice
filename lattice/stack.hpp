#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "convert.hpp"
#include "forwardstack.hpp"
#include "function.hpp"
#include "overload.hpp"
#include "table.hpp"
#include "usertype.hpp"

namespace lat
{
    template <class Path>
    template <class Ret, class... Args>
    Ret IndexedTableView<Path>::invoke(Args&&... args) const
    {
        FunctionView function = *this;
        return function.invokeImpl<false, Ret, Args...>(std::forward<Args>(args)...);
    }

    auto Stack::operator[](auto key)
    {
        return globals()[key];
    }

    template <class... Ret>
    auto Stack::pushFunctionReturning(std::string_view lua, const char* name)
    {
        return pushFunction(lua, name).returning<Ret...>();
    }

    template <class... Ret>
    auto Stack::pushFunctionReturning(const ByteCode& code, const char* name)
    {
        return pushFunction(code, name).returning<Ret...>();
    }

    template <class... Ret>
    auto Stack::execute(std::string_view lua, const char* name)
    {
        auto loaded = pushFunctionReturning<Ret...>(lua, name);
        using R = decltype(loaded)::type;
        return loaded.mFunction.template invokeImpl<false, R>();
    }

    template <class T>
    ObjectView Stack::push(T&& value)
    {
        detail::pushToStack(*this, std::forward<T>(value));
        return getObject(-1);
    }

    template <class T>
    FunctionView Stack::pushFunction(T&& function)
    {
        if constexpr (detail::isOverload<std::remove_reference_t<T>>)
            return pushFunctionImpl(std::forward<T>(function).toFunction());
        else if constexpr (detail::StringViewConstructible<T>)
            return pushFunction(std::string_view(function));
        else
            return pushFunctionImpl(detail::wrapFunction(std::function(function)));
    }

    template <class T, class... Bases>
    UserType Stack::newUserType(std::string_view name)
    {
        return State::getUserTypeRegistry(*this).createUserType<T, Bases...>(*this, name);
    }

    template <class T, class U>
    bool Stack::equal(T&& a, U&& b)
    {
        push(std::forward<T>(a));
        push(std::forward<U>(b));
        const bool eq = same(-2, -1);
        pop(2);
        return eq;
    }
}

#endif

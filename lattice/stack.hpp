#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "convert.hpp"
#include "exception.hpp"
#include "forwardstack.hpp"
#include "function.hpp"
#include "table.hpp"

namespace lat
{
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

    namespace detail
    {
        template <class T>
        inline T pullFunctionArgument(Stack& stack, int& pos)
        {
            using BaseArgT = std::remove_cvref_t<T>;
            if constexpr (std::is_base_of_v<BaseArgT, Stack>)
            {
                static_assert(std::is_lvalue_reference_v<T>, "Stack can only be captured by reference");
                return stack;
            }
            else
            {
                const int index = pos;
                try
                {
                    return detail::pullFromStack<T>(stack, pos);
                }
                catch (const TypeError&)
                {
                    throw ArgumentTypeError(typeid(BaseArgT).name(), index);
                }
            }
        }

        template <class R, class... Args>
        inline std::function<int(Stack&)> wrapFunction(std::function<R(Args...)> function)
        {
            return [function = std::move(function)]([[maybe_unused]] Stack& stack) -> int {
                int argPos = 1;
                auto argValues = std::tuple<Args...>{ detail::pullFunctionArgument<Args>(stack, argPos)... };
                if constexpr (std::is_void_v<R>)
                {
                    std::apply(function, std::move(argValues));
                    return 0;
                }
                else
                {
                    R ret = std::apply(function, std::move(argValues));
                    const int retPos = stack.getTop();
                    if constexpr (detail::Tuple<R>)
                    {
                        constexpr std::size_t size = std::tuple_size_v<R>;
                        if constexpr (size == 0)
                            return 0;
                        else
                        {
                            std::apply(
                                [&](auto&&... retValues) {
                                    (detail::pushToStack(stack, std::forward<decltype(retValues)>(retValues)), ...);
                                },
                                std::move(ret));
                            return stack.getTop() - retPos;
                        }
                    }
                    else
                    {
                        detail::pushToStack(stack, std::move(ret));
                        return stack.getTop() - retPos;
                    }
                }
            };
        }
    }

    template <class T>
    FunctionView Stack::pushFunction(T&& function)
    {
        return pushFunctionImpl(detail::wrapFunction(std::function(function)));
    }
}

#endif

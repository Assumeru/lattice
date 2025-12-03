#ifndef LATTICE_OVERLOAD_H
#define LATTICE_OVERLOAD_H

#include "convert.hpp"
#include "exception.hpp"
#include "forwardstack.hpp"
#include "table.hpp"

#include <array>
#include <tuple>
#include <utility>

namespace lat
{
    template <detail::Function... Funcs>
    class Overload;

    namespace detail
    {
        template <class>
        constexpr inline bool isOverload = false;
        template <class... Types>
        constexpr inline bool isOverload<Overload<Types...>> = true;

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
            static_assert(!detail::IndexedTable<std::remove_cvref_t<R>>, "IndexedTableView's key is likely to dangle");
            return [function = std::move(function)]([[maybe_unused]] Stack& stack) -> int {
                [[maybe_unused]] int argPos = 1;
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

    template <detail::Function... Funcs>
    class Overload
    {
        using StackCheck = bool (*)(Stack&);
        using Wrapped = std::pair<StackCheck, std::function<int(Stack&)>>;

        std::array<Wrapped, sizeof...(Funcs)> mOverloads;

        Overload(const Overload&) = delete;

        template <class T>
        static bool matches([[maybe_unused]] Stack& stack, [[maybe_unused]] int& pos)
        {
            if constexpr (std::is_base_of_v<std::remove_cvref_t<T>, Stack>)
            {
                static_assert(std::is_lvalue_reference_v<T>, "Stack can only be captured by reference");
                return true;
            }
            else
            {
                return pos <= stack.getTop() && detail::stackValueIs<T>(stack, pos);
            }
        }

        template <class R, class... Args>
        static Wrapped wrap(std::function<R(Args...)> function)
        {
            return { []([[maybe_unused]] Stack& stack) -> bool {
                        [[maybe_unused]] int pos = 1;
                        return (true && ... && matches<Args>(stack, pos));
                    },
                detail::wrapFunction(std::move(function)) };
        }

    public:
        explicit Overload(Funcs&&... args)
            : mOverloads{ wrap(std::function(std::forward<Funcs>(args)))... }
        {
            static_assert(sizeof...(Funcs) > 1, "Overloading requires at least two functions");
        }

        std::function<int(Stack&)> toFunction() &&
        {
            return [pairs = std::move(mOverloads)](Stack& stack) -> int {
                for (const Wrapped& pair : pairs)
                {
                    if (pair.first(stack))
                        return pair.second(stack);
                }
                throw std::runtime_error("no matching overload found");
            };
        }
    };

    template <class... Args>
    inline void pushValue(Stack& stack, Overload<Args...>&& overload)
    {
        stack.pushFunction(std::move(overload));
    }
}

#endif

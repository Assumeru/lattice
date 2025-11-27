#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "basicstack.hpp"
#include "convert.hpp"
#include "exception.hpp"
#include "function.hpp"
#include "table.hpp"

namespace lat
{
    // Non-owning lua_State wrapper
    class Stack : public BasicStack
    {
        friend class Reference;
        friend class State;
        friend struct MainStack;

        void call(FunctionRef<void(Stack&)>);

    public:
        explicit Stack(lua_State*);

        auto operator[](auto key) { return globals()[key]; }

        TableView pushArray(int size) { return pushTable(0, size); }

        template <class... Ret>
        auto pushFunctionReturning(std::string_view lua, const char* name = nullptr)
        {
            return pushFunction(lua, name).returning<Ret...>();
        }

        template <class... Ret>
        auto execute(std::string_view lua, const char* name = nullptr)
        {
            auto loaded = pushFunctionReturning<Ret...>(lua, name);
            using R = decltype(loaded)::type;
            return loaded.mFunction.template invokeImpl<false, R>();
        }

        template <class T>
        ObjectView push(T&& value)
        {
            detail::pushToStack(*this, std::forward<T>(value));
            return getObject(-1);
        }

        // TODO remove
        lua_State* get() { return mState; }
    };

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
    FunctionView BasicStack::pushFunction(T&& function)
    {
        return pushFunctionImpl(detail::wrapFunction(std::function(function)));
    }
}

#endif

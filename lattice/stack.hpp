#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "basicstack.hpp"
#include "convert.hpp"
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
            if constexpr (std::is_same_v<BaseArgT, BasicStack> || std::is_same_v<BaseArgT, Stack>)
            {
                static_assert(std::is_lvalue_reference_v<T>, "Stack can only be captured by reference");
                return stack;
            }
            else
            {
                return detail::pullFromStack<T>(stack, pos);
            }
        }

        template <class R, class... Args>
        inline std::function<int(Stack&)> wrapFunction(std::function<R(Args...)> function)
        {
            return [function = std::move(function)](Stack& stack) -> int {
                int argPos = 1;
                auto argValues = std::tuple<Args...>{ detail::pullFunctionArgument<Args>(stack, argPos)... };
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
                else if constexpr (std::is_void_v<R>)
                    return 0;
                else
                {
                    detail::pushToStack(stack, std::move(ret));
                    return stack.getTop() - retPos;
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

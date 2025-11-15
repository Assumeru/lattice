#ifndef LATTICE_FUNCTION_H
#define LATTICE_FUNCTION_H

#include "basicstack.hpp"
#include "convert.hpp"
#include "object.hpp"

namespace lat
{
    class FunctionView
    {
        BasicStack& mStack;
        int mIndex;

        FunctionView(BasicStack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        void cleanUp(int prev);
        void call(int prev, int resCount);

        friend class BasicStack;
        friend class ObjectView;

        template <class>
        constexpr static bool allSpecialized = false;
        template <class... Types>
        constexpr static bool allSpecialized<std::tuple<Types...>> = true && detail::pullsOneValue<Types>...;

        template <class... Types>
        std::tuple<Types...> pullTuple(detail::Type<std::tuple<Types...>>, int pos)
        {
            auto values = std::make_tuple<Types...>(pullFromStack<Types>(mStack, pos)...);
            cleanUp(pos);
            return values;
        }

    public:
        template <class Ret, class... Args>
        Ret invoke(Args&&... args)
        {
            const int top = mStack.getTop();
            constexpr int resCount = [] {
                if constexpr (detail::Tuple<Ret>)
                {
                    constexpr std::size_t size = std::tuple_size_v<Ret>;
                    if constexpr (allSpecialized<Ret>)
                    {
                        static_assert(size <= std::numeric_limits<int>::max());
                        return static_cast<int>(size);
                    }
                    else
                        return -1;
                }
                else if constexpr (std::is_void_v<Ret>)
                    return 0;
                else if constexpr (detail::pullsOneValue<Ret>)
                    return 1;
                else
                    return -1;
            }();
            try
            {
                ObjectView(*this).pushTo(mStack);
                int pos = mStack.getTop();
                (pushToStack(mStack, std::forward<Args>(args)), ...);
                call(pos, resCount);
                if constexpr (resCount != 0)
                {
                    if constexpr (detail::Tuple<Ret>)
                        return pullTuple(detail::Type<Ret>{}, pos);
                    else
                    {
                        Ret value = pullFromStack<Ret>(mStack, pos);
                        cleanUp(pos);
                        return value;
                    }
                }
            }
            catch (...)
            {
                cleanUp(top);
                throw;
            }
        }

        template <class... Args>
        void operator()(Args&&... args)
        {
            invoke<void>(std::forward<Args>(args)...);
        }

        operator ObjectView() noexcept { return ObjectView(mStack, mIndex); }
    };

    inline FunctionView pullValue(BasicStack& stack, int& pos, detail::Type<FunctionView>)
    {
        return stack.getObject(pos++).asFunction();
    }

    inline bool isValue(const BasicStack& stack, int& pos, detail::Type<FunctionView>)
    {
        return stack.isFunction(pos++);
    }

    namespace detail
    {
        template <>
        constexpr inline bool pullsOneValue<FunctionView> = true;
    }
}

#endif

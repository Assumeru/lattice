#ifndef LATTICE_FUNCTION_H
#define LATTICE_FUNCTION_H

#include "convert.hpp"
#include "forwardstack.hpp"
#include "object.hpp"

#include <limits>

namespace lat
{
    template <class>
    class ReturningFunctionView;

    class FunctionReference;
    class TableView;

    class ByteCode
    {
        std::string mCode;
        friend class FunctionView;

        ByteCode() = default;

    public:
        ByteCode(const ByteCode&) = default;
        ByteCode(ByteCode&&) = default;

        std::string_view get() const { return mCode; }
    };

    class FunctionView
    {
        Stack& mStack;
        int mIndex;

        FunctionView(Stack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        void cleanUp(int prev);
        void call(int prev, int resCount);

        friend class ObjectView;
        friend class Stack;

        template <class>
        constexpr static bool allSpecialized = false;

        template <class... Types>
        std::tuple<Types...> pullTuple(Type<std::tuple<Types...>>, int pos)
        {
            // Use braced initializer list to force left-to-right evaluation
            auto values = std::tuple<Types...>{ detail::pullFromStack<Types>(mStack, pos)... };
            cleanUp(pos);
            return values;
        }

        template <bool copy, class Ret, class... Args>
        Ret invokeImpl(Args&&... args)
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
                if constexpr (copy)
                    ObjectView(*this).pushTo(mStack);
                int pos = mStack.getTop();
                (detail::pushToStack(mStack, std::forward<Args>(args)), ...);
                call(pos, resCount);
                if constexpr (resCount != 0)
                {
                    if constexpr (detail::Tuple<Ret>)
                        return pullTuple(Type<Ret>{}, pos);
                    else
                    {
                        Ret value = detail::pullFromStack<Ret>(mStack, pos);
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

    public:
        template <class Ret, class... Args>
        Ret invoke(Args&&... args)
        {
            return invokeImpl<true, Ret>(std::forward<Args>(args)...);
        }

        template <class... Args>
        void operator()(Args&&... args)
        {
            invoke<void>(std::forward<Args>(args)...);
        }

        template <class... Ret>
        auto returning();

        operator ObjectView() noexcept { return ObjectView(mStack, mIndex); }

        FunctionReference store();

        ByteCode dump();

        bool setEnvironment(TableView& environment);
        void setMetatable(TableView& metatable);
        std::optional<ObjectView> pushMetatable();
    };

    // Workaround for GCC < 14 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=71954
    template <class... Types>
    constexpr bool FunctionView::allSpecialized<std::tuple<Types...>> = (true && ... && detail::pullsOneValue<Types>);

    template <class Ret>
    class ReturningFunctionView
    {
        FunctionView mFunction;

        friend class Stack;

    public:
        using type = Ret;

        ReturningFunctionView(FunctionView function)
            : mFunction(function)
        {
        }

        template <class AltRet, class... Args>
        auto invoke(Args&&... args)
        {
            return mFunction.invoke<AltRet>(std::forward<Args>(args)...);
        }

        template <class... Args>
        auto operator()(Args&&... args)
        {
            return mFunction.invoke<Ret>(std::forward<Args>(args)...);
        }

        operator FunctionView() noexcept { return mFunction; }
        operator ObjectView() noexcept { return mFunction; }
    };

    template <class... Ret>
    auto FunctionView::returning()
    {
        constexpr std::size_t count = sizeof...(Ret);
        if constexpr (count == 0)
            return ReturningFunctionView<void>(*this);
        else if constexpr (count == 1)
            return ReturningFunctionView<Ret...>(*this);
        else
            return ReturningFunctionView<std::tuple<Ret...>>(*this);
    }

    inline FunctionView pullValue(Stack& stack, int& pos, Type<FunctionView>)
    {
        return stack.getObject(pos++).asFunction();
    }

    inline bool isValue(const Stack& stack, int& pos, Type<FunctionView>)
    {
        return stack.isFunction(pos++);
    }

    inline FunctionView getValue(ObjectView view, Type<FunctionView>)
    {
        return view.asFunction();
    }

    namespace detail
    {
        template <>
        constexpr inline bool pullsOneValue<FunctionView> = true;

        template <class T, class U = std::remove_reference_t<T>>
        concept ReturningFunction = std::is_same_v<U, ReturningFunctionView<typename U::type>>;
    }

    template <detail::ReturningFunction T>
    inline T pullValue(Stack& stack, int& pos, Type<T>)
    {
        return stack.getObject(pos++).asFunction().returning<typename T::type>();
    }

    template <detail::ReturningFunction T>
    inline bool isValue(const Stack& stack, int& pos, Type<T>)
    {
        return stack.isFunction(pos++);
    }

    template <detail::ReturningFunction T>
    inline T getValue(ObjectView view, Type<T>)
    {
        return view.asFunction().returning<typename T::type>();
    }
}

#endif

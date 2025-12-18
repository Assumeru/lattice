#ifndef LATTICE_CONVERT_H
#define LATTICE_CONVERT_H

#include "forwardstack.hpp"
#include "object.hpp"
#include "state.hpp"
#include "userdata.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

namespace lat
{
    // Sentinel value for template specialization
    template <class T>
    struct Type
    {
        using type = T;
    };

    namespace detail
    {
        template <class T>
        concept Integer = std::is_integral_v<T> && !std::same_as<T, bool>;

        template <class T>
        concept Number = Integer<T> || std::is_floating_point_v<T>;

        template <class T>
        concept Enum = std::is_enum_v<T>;

        template <class T>
        concept Optional = std::same_as<T, std::optional<typename T::value_type>>;

        template <class>
        constexpr inline bool isTuple = false;
        template <class... Types>
        constexpr inline bool isTuple<std::tuple<Types...>> = true;

        template <class T>
        concept Tuple = isTuple<T>;

        template <class>
        constexpr inline bool isVariant = false;
        template <class... Types>
        constexpr inline bool isVariant<std::variant<Types...>> = true;

        template <class T>
        concept Variant = isVariant<T>;

        template <class T>
        concept ReferenceWrapper = !std::is_same_v<T, std::unwrap_reference_t<T>>;

        template <class T>
        concept StringViewConstructible = std::is_constructible_v<std::string_view, T>;

        template <class T>
        concept Function = requires(T&& value) { std::function(std::forward<T>(value)); };

        template <class T>
        concept String = std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>;

        template <class Value, class T = std::remove_cvref_t<Value>>
        inline Value pullFromStack(Stack&, int&);
        template <class Value, class T = std::remove_cvref_t<Value>>
        inline bool stackValueIs(Stack&, int&);

        inline void pushValue(Stack& stack, Nil)
        {
            stack.pushNil();
        }

        template <StringViewConstructible T>
        inline void pushValue(Stack& stack, T&& value)
        {
            stack.pushString(value);
        }

        template <Enum T>
        inline void pushValue(Stack& stack, T&& value)
        {
            using EType = std::underlying_type_t<T>;
            stack.pushInteger(static_cast<EType>(value));
        }

        inline bool isValue(const Stack& stack, int& pos, Type<Nil>)
        {
            return stack.isNil(pos++);
        }

        inline bool isValue(const Stack& stack, int& pos, Type<bool>)
        {
            return stack.isBoolean(pos++);
        }

        template <Number T>
        inline bool isValue(const Stack& stack, int& pos, Type<T>)
        {
            return stack.isNumber(pos++);
        }

        template <String T>
        inline bool isValue(const Stack& stack, int& pos, Type<T>)
        {
            return stack.isString(pos++);
        }

        inline bool isValue(const Stack& stack, int& pos, Type<ObjectView>)
        {
            return stack.getTop() >= pos++;
        }

        template <Optional T>
        inline bool isValue(Stack& stack, int& pos, Type<T>)
        {
            if (stack.getTop() < pos)
                return true;
            else if (stack.isNil(pos))
            {
                ++pos;
                return true;
            }
            using OptT = typename T::value_type;
            return stackValueIs<OptT>(stack, pos);
        }

        template <class... Types>
        inline bool isValue(Stack& stack, int& pos, Type<std::variant<Types...>>)
        {
            const int initial = pos;
            return (false || ... || [&] {
                pos = initial;
                return stackValueIs<Types>(stack, pos);
            }());
        }

        inline Nil getValue(ObjectView view, Type<Nil>)
        {
            return view.asNil();
        }

        inline bool getValue(ObjectView view, Type<bool>)
        {
            return view.asBool();
        }

        template <Integer T>
        inline T getValue(ObjectView view, Type<T>)
        {
            return static_cast<T>(view.asInt());
        }

        template <std::floating_point T>
        inline T getValue(ObjectView view, Type<T>)
        {
            return static_cast<T>(view.asFloat());
        }

        template <String T>
        inline T getValue(ObjectView view, Type<T>)
        {
            return T(view.asString());
        }

        inline ObjectView pullValue(Stack& stack, int& pos, Type<ObjectView>)
        {
            return stack.getObject(pos++);
        }

        inline std::vector<ObjectView> pullValue(Stack& stack, int& pos, Type<std::vector<ObjectView>>)
        {
            const int top = stack.getTop();
            if (pos > top)
                return {};
            std::vector<ObjectView> values;
            values.reserve(static_cast<std::size_t>(top - pos + 1));
            while (pos <= top)
                values.emplace_back(stack.getObject(pos++));
            return values;
        }

        template <Optional T>
        inline T pullValue(Stack& stack, int& pos, Type<T>)
        {
            if (stack.getTop() < pos)
                return {};
            else if (stack.isNil(pos))
            {
                stack.remove(pos);
                return {};
            }
            const int initial = pos;
            using OptT = typename T::value_type;
            if (!stackValueIs<OptT>(stack, pos))
            {
                while (pos > initial)
                {
                    stack.remove(initial);
                    --pos;
                }
                return {};
            }
            pos = initial;
            return pullFromStack<OptT>(stack, pos);
        }

        template <class... Types>
        inline std::variant<Types...> pullValue(Stack& stack, int& pos, Type<std::variant<Types...>>)
        {
            constexpr std::size_t size = sizeof...(Types);
            static_assert(size != 0);
            std::variant<Types...> value;
            std::size_t i = 0;
            (true && ... && [&] {
                int p = pos;
                if (i++ < size - 1 && !stackValueIs<Types>(stack, p))
                    return true;
                value.template emplace<Types>(pullFromStack<Types>(stack, pos));
                return false;
            }());
            return value;
        }

        template <class T>
        concept PushSpecialized = requires(Stack& stack, T&& value) { pushValue(stack, std::forward<T>(value)); };
        template <class T>
        concept PreConvPushSpecialized
            = requires(Stack& stack, T&& value) { pushPreConvValue(stack, std::forward<T>(value)); };

        template <class T>
        concept GetFromViewSpecialized = requires(ObjectView view, Type<T> type) {
            { getValue(view, type) } -> std::constructible_from<T>;
        };

        template <class T>
        concept PullSpecialized = requires(Stack& stack, int& pos, Type<T> type) {
            { pullValue(stack, pos, type) } -> std::constructible_from<T>;
        };

        template <class T>
        concept IsSpecialized = requires(Stack& stack, int& pos, Type<T> type) {
            { isValue(stack, pos, type) } -> std::same_as<bool>;
        };

        template <class T>
        constexpr inline bool pullsOneValue = GetFromViewSpecialized<T>;
        template <>
        constexpr inline bool pullsOneValue<ObjectView> = true;
        template <Optional T>
        constexpr inline bool pullsOneValue<T> = pullsOneValue<typename T::value_type>;
        template <class... Types>
        constexpr inline bool pullsOneValue<std::variant<Types...>> = (true && ... && pullsOneValue<Types>);

        template <class T>
        concept SingleStackPull = pullsOneValue<T> || !PullSpecialized<T>;

        template <class Value, bool light = false, class T = std::remove_cvref_t<Value>,
            class V = std::remove_volatile_t<Value>>
        inline void pushToStack(Stack& stack, Value&& value)
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                stack.pushBoolean(value);
            }
            else if constexpr (std::is_integral_v<T>)
            {
                stack.pushInteger(static_cast<std::ptrdiff_t>(value));
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                stack.pushNumber(static_cast<double>(value));
            }
            else if constexpr (std::is_same_v<T, std::string_view>)
            {
                stack.pushString(value);
            }
            else if constexpr (PreConvPushSpecialized<V>)
            {
                pushPreConvValue(stack, std::forward<V>(value));
            }
            else if constexpr (PushSpecialized<V>)
            {
                pushValue(stack, std::forward<V>(value));
            }
            else if constexpr (Optional<T>)
            {
                if (value)
                {
                    using OptT = typename V::value_type;
                    pushToStack<OptT>(stack, std::forward<OptT>(*value));
                }
                else
                    stack.pushNil();
            }
            else if constexpr (Variant<T>)
            {
                std::visit([&](auto&& v) { pushToStack(stack, v); }, std::forward<V>(value));
            }
            else if constexpr (ReferenceWrapper<T>)
            {
                using RefT = typename T::type;
                pushToStack<RefT&, true>(stack, value.get());
            }
            else if constexpr (std::is_array_v<T>)
                static_assert(false, "array types are not supported");
            else if constexpr (Function<T>)
                stack.pushFunction(std::forward<V>(value));
            else if constexpr (std::is_pointer_v<T>)
            {
                if (value == nullptr)
                    stack.pushNil();
                else
                {
                    using RefT = std::add_lvalue_reference_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
                    pushToStack<RefT, true>(stack, std::forward<RefT>(*value));
                }
            }
            else if constexpr (std::is_same_v<T, ObjectView>)
            {
                if constexpr (std::is_const_v<V>)
                    static_assert(false, "cannot push const object");
                value.pushTo(stack);
            }
            else if constexpr (std::is_convertible_v<T, ObjectView>)
            {
                if constexpr (std::is_const_v<V>)
                    static_assert(false, "cannot push const object");
                ObjectView(value).pushTo(stack);
            }
            else if constexpr (light)
            {
                State::getUserTypeRegistry(stack).pushPointer(stack, std::forward<Value>(value));
            }
            else
            {
                State::getUserTypeRegistry(stack).pushValue(stack, std::forward<Value>(value));
            }
        }

        template <class Value, class T>
        inline bool stackValueIs(Stack& stack, int& pos)
        {
            if constexpr (IsSpecialized<T>)
            {
                return isValue(stack, pos, Type<T>{});
            }
            else if constexpr (ReferenceWrapper<T>)
            {
                using RefT = typename T::type;
                return State::getUserTypeRegistry(stack).matches<RefT>(stack, pos++);
            }
            else
            {
                return State::getUserTypeRegistry(stack).matches<Value>(stack, pos++);
            }
        }

        template <class Value, class T>
        inline Value pullFromStack(Stack& stack, int& pos)
        {
            if constexpr (PullSpecialized<T>)
            {
                return pullValue(stack, pos, Type<T>{});
            }
            else if constexpr (GetFromViewSpecialized<T>)
            {
                T value = getValue(stack.getObject(pos), Type<T>{});
                stack.remove(pos);
                return value;
            }
            else
            {
                Value value = stack.getObject(pos).as<Value>();
                stack.remove(pos);
                return value;
            }
        }
    }

    template <class T>
    bool ObjectView::is() const
    {
        int pos = mIndex;
        return detail::stackValueIs<T>(mStack, pos) && mIndex - pos <= 1;
    }

    template <class Value>
    Value ObjectView::as() const
    {
        using T = std::remove_cvref_t<Value>;
        if constexpr (std::is_same_v<T, ObjectView>)
        {
            return *this;
        }
        else if constexpr (detail::Optional<T>)
        {
            if (isNil())
                return {};
            using OptT = typename T::value_type;
            if (!is<OptT>())
                return {};
            return detail::getValue(*this, Type<OptT>{});
        }
        else if constexpr (detail::GetFromViewSpecialized<T>)
        {
            return detail::getValue(*this, Type<T>{});
        }
        else if constexpr (detail::ReferenceWrapper<T>)
        {
            using RefT = typename T::type;
            auto* value = State::getUserTypeRegistry(mStack).as<RefT>(mStack, mIndex);
            return std::forward<Value>(*value);
        }
        else
        {
            auto* value = State::getUserTypeRegistry(mStack).as<Value>(mStack, mIndex);
            if constexpr (std::is_pointer_v<T>)
                return static_cast<Value>(value);
            else
                return std::forward<Value>(*value);
        }
    }
}

#endif

#ifndef LATTICE_CONVERT_H
#define LATTICE_CONVERT_H

#include "basicstack.hpp"
#include "object.hpp"

#include <cstddef>
#include <optional>
#include <string_view>
#include <type_traits>

namespace lat
{
    namespace detail
    {
        template <class T>
        concept Optional = std::same_as<T, std::optional<typename T::value_type>>;

        template <class T>
        constexpr inline bool isReferenceWrapper = !std::is_same_v<T, std::unwrap_reference_t<T>>;

        template <class T>
        concept PushSpecialization
            = requires(BasicStack& stack, T&& value) { pushValue(stack, std::forward<T>(value)); };
    }

    template <class T, typename = std::enable_if_t<std::is_constructible_v<std::string_view, T>>>
    inline void pushValue(BasicStack& stack, T&& value)
    {
        stack.pushString(value);
    }

    template <class Value, class T = std::remove_cv_t<Value>>
    void pushToStack(BasicStack& stack, Value&& value)
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
        else if constexpr (std::is_same_v<std::remove_reference_t<T>, std::string_view>)
        {
            stack.pushString(value);
        }
        else if constexpr (detail::PushSpecialization<std::remove_volatile_t<Value>>)
        {
            pushValue(stack, std::forward<Value>(value));
        }
        else if constexpr (detail::Optional<T>)
        {
            if (value)
            {
                using OptT = typename Value::value_type;
                pushToStack<OptT>(stack, std::forward<OptT>(*value));
            }
            else
                stack.pushNil();
        }
        else if constexpr (detail::isReferenceWrapper<T>)
        {
            pushToStack(value.get());
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            if (value == nullptr)
                stack.pushNil();
            else
            {
                using RefT = std::add_lvalue_reference_t<std::remove_pointer_t<Value>>;
                pushToStack<RefT>(std::forward<RefT>(*value));
            }
        }
        else if constexpr (std::is_same_v<T, ObjectView>)
        {
            if constexpr (std::is_const_v<Value>)
                static_assert(false, "cannot push const object");
            value.pushTo(stack);
        }
        else if constexpr (std::is_convertible_v<T, ObjectView>)
        {
            if constexpr (std::is_const_v<Value>)
                static_assert(false, "cannot push const object");
            ObjectView(value).pushTo(stack);
        }
        else if constexpr (std::is_lvalue_reference_v<T>)
        {
            // TODO push light user data
            static_assert(false, "light user data not implemented");
        }
        else
        {
            // TODO push user data
            static_assert(false, "user data not implemented");
        }
    }
}

#endif

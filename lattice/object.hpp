#ifndef LATTICE_OBJECT_H
#define LATTICE_OBJECT_H

#include <concepts>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace lat
{
    class Stack;
    enum class LuaType : int;

    class ObjectView
    {
        Stack& mStack;
        int mIndex;

        template<class T, class U>
        static constexpr bool matches = std::is_same_v<T, std::remove_cvref_t<U>>;
        template<class T>
        static constexpr bool isOptional = std::is_same_v<std::remove_cvref_t<T>, std::optional<typename T::value_type>>;

    public:
        ObjectView(Stack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        bool isNil() const;
        LuaType getType() const;

        bool asBool() const;
        std::ptrdiff_t asInt() const;
        double asFloat() const;
        std::string_view asString() const;

        template <class T, std::enable_if_t<matches<T, bool>>>
        bool as() const
        {
            return asBool();
        }
        template <class T, std::enable_if_t<matches<T, std::string_view>>>
        std::string_view as() const
        {
            return asString();
        }
        template <class T, std::enable_if_t<matches<T, std::string>>>
        std::string as() const
        {
            return std::string(asString());
        }
        template <std::integral T, typename = std::enable_if_t<!matches<T, bool>>>
        T as() const
        {
            return static_cast<T>(asInt());
        }
        template <std::floating_point T>
        T as() const
        {
            return static_cast<T>(asFloat());
        }

        template <class T, class V = T::value_type>
        std::optional<V> as() const
        {
            if (isNil())
                return {};
            return as<V>();
        }

        ObjectView operator[](std::string_view key) const;
        ObjectView operator[](std::ptrdiff_t index) const;
    };
}

#endif

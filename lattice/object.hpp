#ifndef LATTICE_OBJECT_H
#define LATTICE_OBJECT_H

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

        template <class T>
        static constexpr bool isOptional = std::is_same_v<T, std::optional<typename T::value_type>>;

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

        template <class T>
        T as() const
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                return asBool();
            }
            else if constexpr (std::is_integral_v<T>)
            {
                return static_cast<T>(asInt());
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                return static_cast<T>(asFloat());
            }
            else if constexpr (std::is_same_v<T, std::string_view>)
            {
                return asString();
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                return std::string(asString());
            }
            else if constexpr (isOptional<T>)
            {
                if (isNil())
                    return {};
                return as<V>();
            }
            else
            {
                static_assert(false);
            }
        }

        ObjectView operator[](std::string_view key);
        ObjectView operator[](std::ptrdiff_t index);
        ObjectView operator[](const ObjectView& key);
    };
}

#endif

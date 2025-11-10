#ifndef LATTICE_OBJECT_H
#define LATTICE_OBJECT_H

#include "table.hpp"

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

        friend class IndexedTableView;
        friend class TableView;

    public:
        ObjectView(Stack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        bool isNil() const;
        bool isBoolean() const;
        bool isNumber() const;
        bool isString() const;
        bool isTable() const;
        LuaType getType() const;

        bool asBool() const;
        std::ptrdiff_t asInt() const;
        double asFloat() const;
        std::string_view asString() const;
        TableView asTable() const;

        template <class T>
        bool is() const
        {
            if constexpr (std::is_same_v<T, bool>)
            {
                return isBoolean();
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                return isNumber();
            }
            else if constexpr (std::is_same_v<T, std::string_view>)
            {
                return isString();
            }
            else if constexpr (std::is_same_v<T, std::string>)
            {
                return isString();
            }
            else if constexpr (std::is_same_v<T, TableView>)
            {
                return isTable();
            }
            else
            {
                static_assert(false);
            }
        }

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
            else if constexpr (std::is_same_v<T, TableView>)
            {
                return asTable();
            }
            else if constexpr (isOptional<T>)
            {
                if (!is<typename T::value_type>())
                    return {};
                return as<typename T::value_type>();
            }
            else
            {
                static_assert(false);
            }
        }
    };
}

#endif

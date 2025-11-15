#ifndef LATTICE_OBJECT_H
#define LATTICE_OBJECT_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>

namespace lat
{
    namespace detail
    {
        struct Nil
        {
        };

        inline bool operator==(const Nil&, const Nil&)
        {
            return true;
        }
    }

    constexpr inline detail::Nil nil{};

    class BasicStack;
    class FunctionView;
    class TableView;
    enum class LuaType : int;

    class ObjectView
    {
        BasicStack& mStack;
        int mIndex;

        template <class T>
        static constexpr bool isOptional = std::is_same_v<T, std::optional<typename T::value_type>>;

    public:
        ObjectView(BasicStack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        bool isNil() const;
        bool isBoolean() const;
        bool isNumber() const;
        bool isString() const;
        bool isTable() const;
        bool isFunction() const;
        LuaType getType() const;

        detail::Nil asNil() const;
        bool asBool() const;
        std::ptrdiff_t asInt() const;
        double asFloat() const;
        std::string_view asString() const;
        TableView asTable() const;
        FunctionView asFunction() const;

        ObjectView pushTo(BasicStack&);

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

    inline bool operator==(const detail::Nil&, const ObjectView& object)
    {
        return object.isNil();
    }
}

#endif

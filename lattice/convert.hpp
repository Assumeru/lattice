#ifndef LATTICE_CONVERT_H
#define LATTICE_CONVERT_H

#include "basicstack.hpp"
#include "object.hpp"

#include <concepts>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <type_traits>
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

        template <class T>
        constexpr inline bool isReferenceWrapper = !std::is_same_v<T, std::unwrap_reference_t<T>>;

        template <class T>
        concept StringViewConstructible = std::is_constructible_v<std::string_view, T>;

        inline void pushValue(BasicStack& stack, Nil)
        {
            stack.pushNil();
        }

        template <StringViewConstructible T>
        inline void pushValue(BasicStack& stack, T&& value)
        {
            stack.pushString(value);
        }

        template <Enum T>
        inline void pushValue(BasicStack& stack, T&& value)
        {
            using EType = std::underlying_type_t<T>;
            stack.pushInteger(static_cast<EType>(value));
        }

        inline bool isValue(const BasicStack& stack, int& pos, Type<Nil>)
        {
            return stack.isNil(pos++);
        }

        inline bool isValue(const BasicStack& stack, int& pos, Type<bool>)
        {
            return stack.isBoolean(pos++);
        }

        template <Number T>
        inline bool isValue(const BasicStack& stack, int& pos, Type<T>)
        {
            return stack.isNumber(pos++);
        }

        template <std::constructible_from<std::string_view> T>
        inline bool isValue(const BasicStack& stack, int& pos, Type<T>)
        {
            return stack.isString(pos++);
        }

        inline bool isValue(const BasicStack& stack, int& pos, Type<ObjectView>)
        {
            return stack.getTop() >= pos++;
        }

        template <Optional T>
        inline bool isValue(const BasicStack& stack, int& pos, Type<T>)
        {
            if (stack.isNil(pos))
            {
                ++pos;
                return true;
            }
            using OptT = typename T::value_type;
            return stackValueIs<OptT>(stack, pos);
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

        template <std::constructible_from<std::string_view> T>
        inline T getValue(ObjectView view, Type<T>)
        {
            return T(view.asString());
        }

        inline ObjectView pullValue(BasicStack& stack, int& pos, Type<ObjectView>)
        {
            return stack.getObject(pos++);
        }

        inline std::vector<ObjectView> pullValue(BasicStack& stack, int& pos, Type<std::vector<ObjectView>>)
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

        template <class T>
        concept PushSpecialized = requires(BasicStack& stack, T&& value) { pushValue(stack, std::forward<T>(value)); };
        template <class T>
        concept PreConvPushSpecialized
            = requires(BasicStack& stack, T&& value) { pushPreConvValue(stack, std::forward<T>(value)); };

        template <class T>
        concept GetFromViewSpecialized = requires(ObjectView view, Type<T> type) {
            { getValue(view, type) } -> std::constructible_from<T>;
        };

        template <class T>
        concept PullSpecialized = requires(BasicStack& stack, int& pos, Type<T> type) {
            { pullValue(stack, pos, type) } -> std::constructible_from<T>;
        };

        template <class T>
        concept IsSpecialized = requires(const BasicStack& stack, int& pos, Type<T> type) {
            { isValue(stack, pos, type) } -> std::same_as<bool>;
        };

        template <class T>
        constexpr inline bool pullsOneValue = GetFromViewSpecialized<T>;
        template <>
        constexpr inline bool pullsOneValue<ObjectView> = true;

        template <class T>
        concept SingleStackPull = pullsOneValue<T>;

        template <class T>
        void destroyUserData(void* userdata)
        {
            std::destroy_at(static_cast<T*>(userdata));
        }

        template <class Value, bool light = false, class T = std::remove_cvref_t<Value>,
            class V = std::remove_volatile_t<Value>>
        inline void pushToStack(BasicStack& stack, Value&& value)
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
            else if constexpr (isReferenceWrapper<T>)
            {
                using RefT = typename V::type;
                pushToStack<RefT&, true>(stack, value.get());
            }
            else if constexpr (std::is_pointer_v<T>)
            {
                if (value == nullptr)
                    stack.pushNil();
                else
                {
                    using RefT = std::add_lvalue_reference_t<std::remove_pointer_t<V>>;
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
            else
            {
                constexpr auto pushType = [](BasicStack& stack, std::size_t size, void* pointer) {
                    TableView metatable = stack.pushMetatable(std::type_index(typeid(T)), &destroyUserData<T>);
                    std::span<std::byte> data = stack.pushUserData(size);
                    std::memcpy(data.data(), &pointer, sizeof(void*));
                    stack.getObject(-1).setMetatable(metatable);
                    stack.remove(-2);
                    return data;
                };
                constexpr std::size_t pointerSize = sizeof(void*);
                if constexpr (light)
                {
                    // Light user data cannot have a unique metatable so we push a full user data the size of a pointer
                    pushType(stack, pointerSize, std::addressof(value));
                }
                else
                {
                    // object + padding to ensure alignment
                    constexpr std::size_t dataSize = sizeof(T) + alignof(T) - 1;
                    // Push a user type with the correct metatable. We init to nullptr because construction might throw
                    std::span<std::byte> data = pushType(stack, pointerSize + dataSize, nullptr);
                    void* pointer = data.data() + pointerSize;
                    std::size_t size = dataSize;
                    pointer = std::align(alignof(T), dataSize, pointer, size);
                    try
                    {
                        if (pointer == nullptr)
                            throw std::runtime_error(std::string("failed to align object of type ") + typeid(T).name());
                        pointer = new (pointer) T(std::forward<Value>(value));
                        std::memcpy(data.data(), &pointer, sizeof(void*));
                    }
                    catch (...)
                    {
                        stack.pop();
                        throw;
                    }
                }
            }
        }

        template <class Value, class T = std::remove_cvref_t<Value>>
        inline bool stackValueIs(const BasicStack& stack, int& pos)
        {
            if constexpr (std::is_reference_v<Value>)
            {
                static_assert(false, "a stack value is not a reference");
            }
            else if constexpr (IsSpecialized<T>)
            {
                return isValue(stack, pos, Type<T>{});
            }
            else
            {
                static_assert(false, "TODO");
            }
        }

        template <Optional T>
        inline T pullValue(BasicStack& stack, int& pos, Type<T>)
        {
            if (stack.isNil(pos))
            {
                stack.remove(pos);
                return {};
            }
            int p = pos;
            using OptT = typename T::value_type;
            if (!stackValueIs<OptT>(stack, p))
                return {};
            return pullFromStack<OptT>(stack, pos);
        }

        template <class Value, class T = std::remove_cvref_t<Value>>
        inline Value pullFromStack(BasicStack& stack, int& pos)
        {
            if constexpr (std::is_reference_v<Value>)
            {
                static_assert(false, "a stack value is not a reference");
            }
            else if constexpr (PullSpecialized<T>)
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
                static_assert(false, "TODO");
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
        if constexpr (std::is_reference_v<Value>)
        {
            static_assert(false, "a stack value is not a reference");
        }
        else if constexpr (std::is_same_v<T, ObjectView>)
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
        else
        {
            static_assert(false, "TODO");
        }
    }
}

#endif

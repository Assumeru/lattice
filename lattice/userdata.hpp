#ifndef LATTICE_USERDATA_H
#define LATTICE_USERDATA_H

#include "functionref.hpp"
#include "object.hpp"
#include "reference.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

namespace lat
{
    class Stack;
    class UserType;

    using UserDataDestructor = void (*)(Stack&, ObjectView);

    namespace detail
    {
        template <class T>
        concept UnqualifiedType = std::same_as<std::remove_cvref_t<std::remove_pointer_t<T>>, T>;

        template <class T, class B>
        concept BaseType = std::derived_from<B, T>;

        using TypeCaster = void* (*)(void*);

        template <class T, BaseType<T> B>
        inline void* baseCast(void* pointer)
        {
            return static_cast<B*>(static_cast<T*>(pointer));
        }
    };

    struct UserTypeData
    {
        TableReference mMetatable;
        std::vector<std::tuple<std::type_index, detail::TypeCaster>> mDerived;

        UserTypeData(TableReference&& ref)
            : mMetatable(std::move(ref))
        {
        }
    };

    class UserTypeRegistry
    {
        std::unordered_map<std::type_index, UserTypeData> mMetatables;
        FunctionReference mDefaultIndex;
        FunctionReference mDefaultNewIndex;

        friend struct MainStack;

        void clear();

        static constexpr std::size_t pointerSize = sizeof(void*);

        using Destructor = void (*)(void*);

        static void destroyUserData(ObjectView view, Destructor);

        template <class T>
        static void destroyUserData(Stack&, ObjectView view)
        {
            destroyUserData(view, [](void* pointer) { std::destroy_at(static_cast<T*>(pointer)); });
        }

        UserTypeData& getUserTypeData(Stack&, std::type_index, UserDataDestructor);

        std::span<std::byte> pushUserData(Stack&, std::size_t, const void*, std::type_index, UserDataDestructor);

        void pushUserData(
            Stack&, std::size_t, std::size_t, const std::type_info&, UserDataDestructor, FunctionRef<void*(void*)>);

        bool matches(Stack&, int, std::type_index) const;

        void* getUserData(Stack&, int, const std::type_info&) const;

        UserType createUserType(Stack&, std::type_index, UserDataDestructor, std::string_view);

    public:
        template <class Value, class T = std::remove_cvref_t<Value>>
        void pushPointer(Stack& stack, Value&& value)
        {
            static_assert(!std::is_pointer_v<T>);
            // Light user data cannot have a unique metatable so we push a full user data the size of a pointer
            pushUserData(stack, pointerSize, std::addressof(value), std::type_index(typeid(T)), &destroyUserData<T>);
        }

        template <class Value, class T = std::remove_cvref_t<Value>>
        void pushValue(Stack& stack, Value&& value)
        {
            static_assert(!std::is_pointer_v<T>);
            pushUserData(stack, sizeof(T), alignof(T), typeid(T), &destroyUserData<T>,
                [&](void* pointer) { return new (pointer) T(std::forward<Value>(value)); });
        }

        template <class Value, class T = std::remove_cvref_t<std::remove_pointer_t<Value>>>
        bool matches(Stack& stack, int index) const
        {
            return matches(stack, index, std::type_index(typeid(T)));
        }

        template <class Value, class T = std::remove_cvref_t<std::remove_pointer_t<Value>>>
        T* as(Stack& stack, int index) const
        {
            void* value = getUserData(stack, index, typeid(T));
            return static_cast<T*>(value);
        }

        template <detail::UnqualifiedType T, detail::BaseType<T>... Bases>
        UserType createUserType(Stack& stack, std::string_view name);
    };
}

#endif

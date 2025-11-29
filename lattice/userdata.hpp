#ifndef LATTICE_USERDATA_H
#define LATTICE_USERDATA_H

#include "functionref.hpp"
#include "object.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace lat
{
    class Stack;
    class TableReference;

    using UserDataDestructor = void (*)(Stack&, ObjectView);

    class UserTypeRegistry
    {
        std::unordered_map<std::type_index, TableReference> mMetatables;

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

        const TableReference& getMetatable(Stack&, std::type_index, UserDataDestructor);

        std::span<std::byte> pushUserData(Stack&, std::size_t, const void*, std::type_index, UserDataDestructor);

        void pushUserData(
            Stack&, std::size_t, std::size_t, const std::type_info&, UserDataDestructor, FunctionRef<void*(void*)>);

        bool matches(const Stack&, int, std::type_index) const;

        void* getUserData(Stack&, int, const std::type_info&) const;

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
        bool matches(const Stack& stack, int index) const
        {
            return matches(stack, index, std::type_index(typeid(T)));
        }

        template <class Value, class T = std::remove_cvref_t<std::remove_pointer_t<Value>>>
        T* as(Stack& stack, int index) const
        {
            void* value = getUserData(stack, index, typeid(T));
            return static_cast<T*>(value);
        }
    };
}

#endif

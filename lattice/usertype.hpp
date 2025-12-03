#ifndef LATTICE_USERTYPE_H
#define LATTICE_USERTYPE_H

#include "forwardstack.hpp"
#include "overload.hpp"
#include "reference.hpp"
#include "table.hpp"
#include "userdata.hpp"

#include <string_view>

namespace lat
{
    class IndexedUserType;

    namespace detail
    {
        template <class Type, class T = std::remove_cvref_t<Type>>
        concept PropertyFunction
            = Function<T> || isOverload<T> || std::is_same_v<FunctionView, T> || std::is_same_v<FunctionReference, T>;
    }

    class UserType
    {
        Stack& mStack;
        const TableReference& mMetatable;
        const FunctionReference& mDefaultIndex;
        const FunctionReference& mDefaultNewIndex;

        UserType(const UserType&) = delete;
        UserType(UserType&&) = default;

        UserType(Stack&, const TableReference&, const FunctionReference&, const FunctionReference&);

        friend class UserTypeRegistry;

        TableView props() const;
        TableView props(std::string_view) const;
        TableView getters() const;
        TableView setters() const;

    public:
        IndexedUserType operator[](std::string_view);

        template <class K, detail::PropertyFunction V>
        void setReadOnlyProperty(K&& key, V&& value)
        {
            setProperty(std::forward<K>(key), std::forward<V>(value), mDefaultNewIndex);
        }

        template <class K, detail::PropertyFunction V>
        void setWriteOnlyProperty(K&& key, V&& value)
        {
            setProperty(std::forward<K>(key), mDefaultIndex, std::forward<V>(value));
        }

        template <class K, detail::PropertyFunction G, detail::PropertyFunction S>
        void setProperty(K&& key, G&& getter, S&& setter)
        {
            getters().set(std::forward<G>(getter), std::forward<K>(key));
            mStack.pop();
            setters().set(std::forward<S>(setter), std::forward<K>(key));
            mStack.pop();
        }

        template <class K, class V>
        void set(K&& key, V&& value)
        {
            if constexpr (detail::StringViewConstructible<std::remove_cvref_t<K>>)
                props(std::forward<K>(key)).set(std::forward<V>(value), std::forward<K>(key));
            else
                props().set(std::forward<V>(value), std::forward<K>(key));
            mStack.pop();
        }
    };

    class IndexedUserType
    {
        UserType& mType;
        std::string_view mKey;

        IndexedUserType(const IndexedUserType&) = delete;
        IndexedUserType(IndexedUserType&&) = delete;

        IndexedUserType(UserType& type, std::string_view key)
            : mType(type)
            , mKey(key)
        {
        }

        friend class UserType;

    public:
        template <class T>
        void operator=(T&& value)
        {
            mType.set(mKey, std::forward<T>(value));
        }
    };

    template <detail::UnqualifiedType T, detail::BaseType<T>... Bases>
    UserType UserTypeRegistry::createUserType(Stack& stack, std::string_view name)
    {
        const auto& type = typeid(T);
        UserType created = createUserType(stack, type, &destroyUserData<T>, name);
        (getUserTypeData(stack, typeid(Bases), &destroyUserData<Bases>)
                .mDerived.emplace_back(type, &detail::baseCast<T, Bases>),
            ...);
        return created;
    }
}

#endif

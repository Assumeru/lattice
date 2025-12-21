#ifndef LATTICE_OBJECT_H
#define LATTICE_OBJECT_H

#include <cstddef>
#include <iosfwd>
#include <optional>
#include <span>
#include <string_view>

namespace lat
{
    struct Nil
    {
    };

    inline bool operator==(const Nil&, const Nil&)
    {
        return true;
    }

    constexpr inline Nil nil{};

    class FunctionView;
    class Reference;
    class Stack;
    class TableLikeView;
    class TableView;
    enum class LuaType : int;

    class ObjectView
    {
        Stack& mStack;
        int mIndex;

        friend class Reference;

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
        bool isFunction() const;
        bool isCoroutine() const;
        bool isUserData() const;
        bool isLightUserData() const;
        LuaType getType() const;
        std::string_view getTypeName() const;

        Nil asNil() const;
        bool asBool() const;
        std::ptrdiff_t asInt() const;
        double asFloat() const;
        std::string_view asString() const;
        TableView asTable() const;
        TableLikeView asTableLike() const;
        FunctionView asFunction() const;
        std::span<std::byte> asUserData() const;
        void* asLightUserData() const;

        ObjectView pushTo(Stack&);

        bool setEnvironment(TableView& environment);
        void setMetatable(TableView& metatable);
        std::optional<TableView> pushMetatable();

        Reference store();

        template <class T>
        bool is() const;

        template <class T>
        T as() const;

        friend std::ostream& operator<<(std::ostream& stream, ObjectView value);
    };

    inline bool operator==(const Nil&, const ObjectView& object)
    {
        return object.isNil();
    }
}

#endif

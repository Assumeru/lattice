#ifndef LATTICE_OBJECT_H
#define LATTICE_OBJECT_H

#include <cstddef>
#include <iosfwd>
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

    class BasicStack;
    class FunctionView;
    class TableView;
    enum class LuaType : int;

    class ObjectView
    {
        BasicStack& mStack;
        int mIndex;

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

        Nil asNil() const;
        bool asBool() const;
        std::ptrdiff_t asInt() const;
        double asFloat() const;
        std::string_view asString() const;
        TableView asTable() const;
        FunctionView asFunction() const;

        ObjectView pushTo(BasicStack&);

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

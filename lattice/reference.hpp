#ifndef LATTICE_REFERENCE_H
#define LATTICE_REFERENCE_H

#include "functionref.hpp"

struct lua_State;

namespace lat
{
    class FunctionView;
    class ObjectView;
    class Stack;
    class TableView;

    class FunctionReference;
    class TableReference;

    class Reference
    {
        lua_State* mState;
        int mRef;

        Reference(const Reference&) = delete;

    public:
        Reference();

        Reference(lua_State& state, int ref)
            : mState(&state)
            , mRef(ref)
        {
        }

        Reference(Reference&&);
        Reference(FunctionReference&&);
        Reference(TableReference&&);

        Reference& operator=(Reference&&);
        Reference& operator=(FunctionReference&&);
        Reference& operator=(TableReference&&);

        friend void swap(Reference&, Reference&);

        ~Reference();

        void reset();
        bool isValid() const;

        ObjectView pushTo(Stack&) const;
        void onStack(FunctionRef<void(Stack&, ObjectView)>);

        void operator=(ObjectView&);
        ObjectView operator=(ObjectView&&);

        friend bool operator==(const Reference&, const Reference&);
    };

    class FunctionReference
    {
        Reference mReference;

        FunctionReference(Reference&& ref);

        friend struct MainStack;
        friend class Reference;
        friend class FunctionView;

    public:
        FunctionReference() = default;

        void reset();
        bool isValid() const;

        FunctionView pushTo(Stack&) const;
        void onStack(FunctionRef<void(Stack&, FunctionView)>);

        void operator=(FunctionView&);
        FunctionView operator=(FunctionView&&);

        friend bool operator==(const Reference&, const FunctionReference&);
        friend bool operator==(const FunctionReference&, const FunctionReference&);
        friend bool operator==(const TableReference&, const FunctionReference&);
    };

    class TableReference
    {
        Reference mReference;

        TableReference(Reference&&);

        friend class Reference;
        friend class TableView;

    public:
        TableReference() = default;

        void reset();
        bool isValid() const;

        TableView pushTo(Stack&) const;
        void onStack(FunctionRef<void(Stack&, TableView)>);

        void operator=(TableView&);
        TableView operator=(TableView&&);

        friend bool operator==(const Reference&, const TableReference&);
        friend bool operator==(const TableReference&, const TableReference&);
        friend bool operator==(const TableReference&, const FunctionReference&);
    };

    void pushValue(Stack&, const Reference&);
    void pushValue(Stack&, const FunctionReference&);
    void pushValue(Stack&, const TableReference&);
}

#endif

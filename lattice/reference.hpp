#ifndef LATTICE_REFERENCE_H
#define LATTICE_REFERENCE_H

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

        ObjectView pushTo(Stack&) const;

        void operator=(ObjectView&);
        ObjectView operator=(ObjectView&&);
    };

    class FunctionReference
    {
        Reference mReference;

        FunctionReference(Reference&& ref);

        friend class Reference;
        friend class FunctionView;

    public:
        void reset();

        FunctionView pushTo(Stack&) const;

        void operator=(FunctionView&);
        FunctionView operator=(FunctionView&&);
    };

    class TableReference
    {
        Reference mReference;

        TableReference(Reference&&);

        friend class Reference;
        friend class TableView;

    public:
        void reset();

        TableView pushTo(Stack&) const;

        void operator=(TableView&);
        TableView operator=(TableView&&);
    };
}

#endif

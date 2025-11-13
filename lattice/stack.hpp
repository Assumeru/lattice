#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "basicstack.hpp"
#include "table.hpp"

namespace lat
{
    // Non-owning lua_State wrapper
    class Stack : public BasicStack
    {
        friend class State;
        friend struct MainStack;

        void call(FunctionRef<void(Stack&)>);

    public:
        explicit Stack(lua_State*);

        auto operator[](auto key) { return globals()[key]; }

        TableView pushArray(int size) { return pushTable(0, size); }

        // TODO remove
        lua_State* get() { return mState; }
    };
}

#endif

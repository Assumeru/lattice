#ifndef LATTICE_STACK_H
#define LATTICE_STACK_H

#include "basicstack.hpp"
#include "convert.hpp"
#include "function.hpp"
#include "table.hpp"

namespace lat
{
    // Non-owning lua_State wrapper
    class Stack : public BasicStack
    {
        friend class Reference;
        friend class State;
        friend struct MainStack;

        void call(FunctionRef<void(Stack&)>);

    public:
        explicit Stack(lua_State*);

        auto operator[](auto key) { return globals()[key]; }

        TableView pushArray(int size) { return pushTable(0, size); }

        template <class... Ret>
        auto pushFunctionReturning(std::string_view lua, const char* name = nullptr)
        {
            return pushFunction(lua, name).returning<Ret...>();
        }

        template <class... Ret>
        auto execute(std::string_view lua, const char* name = nullptr)
        {
            auto loaded = pushFunctionReturning<Ret...>(lua, name);
            using R = decltype(loaded)::type;
            return loaded.mFunction.template invokeImpl<false, R>();
        }

        template <class T>
        ObjectView push(T&& value)
        {
            detail::pushToStack(*this, std::forward<T>(value));
            return getObject(-1);
        }

        // TODO remove
        lua_State* get() { return mState; }
    };
}

#endif

#ifndef LATTICE_STATE_H
#define LATTICE_STATE_H

#include "functionref.hpp"

#include <cstddef>
#include <memory>
#include <span>
#include <typeindex>

struct lua_Debug;

namespace lat
{
    template <class UserData>
    using Allocator = void* (*)(UserData*, void*, std::size_t, std::size_t);

    enum class LuaHookMask : int;
    class ObjectView;
    struct MainStack;
    class Stack;
    class TableReference;

    enum class Library
    {
        Base,
        Package,
        String,
        Table,
        Math,
        IO,
        OS,
        Debug,
        Bit,
        JIT,
        FFI,
        StringBuffer,
    };

    // Owning lua_State wrapper.
    class State
    {
        std::unique_ptr<MainStack> mState;

        friend class BasicStack;

        static Stack& getMain(BasicStack&);
        static const TableReference& getMetatable(BasicStack&, const std::type_index&, void(Stack&, ObjectView));

    public:
        State();
        State(Allocator<void>, void*);

        template <class UserData>
        State(Allocator<UserData> allocator, UserData* userData)
            : State(reinterpret_cast<Allocator<void>>(allocator), static_cast<void*>(userData))
        {
        }

        ~State();

        void withStack(FunctionRef<void(Stack&)>) const;

        void setDebugHook(FunctionRef<void(Stack&, lua_Debug&)> hook, LuaHookMask mask, int count = 0) const;
        void disableDebugHook() const;

        void loadLibraries(std::span<const Library> = {}) const;
    };
}

#endif

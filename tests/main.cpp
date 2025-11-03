#include <lua/api.hpp>
#include <state.hpp>

#include <iostream>

namespace
{
    struct AllocatorData
    {
        int mStackSize = 0;
        bool mBlock = false;
    };

    void* allocate(AllocatorData* state, void* ptr, std::size_t oSize, std::size_t nSize)
    {
        if (state->mBlock && nSize > oSize)
            return nullptr;
        std::cout << "Calling allocate " << ptr << " osize: " << oSize << " nSize: " << nSize
                  << " stack size: " << state->mStackSize << '\n';
        if (nSize == 0)
        {
            std::free(ptr);
            return nullptr;
        }
        return std::realloc(ptr, nSize);
    }
}

int main(int, const char**)
{
    AllocatorData allocatorData;
    lat::State state(allocate, &allocatorData);
    lat::LuaApi api(*state.get());

    api.runGarbageCollector();
    std::cout << "Stack size: " << api.getStackSize() << '\n';
    // api.setStackSize(1);
    std::cout << "Stack size: " << api.getStackSize() << '\n';
    // allocatorData.mBlock = true;
    auto status = api.protectedCall(
        [](lua_State* s) -> int {
            lat::LuaApi pState(*s);
            lua_State* main = static_cast<lua_State*>(pState.asUserData(-1));
            std::cout << "Protected state equals main state: " << (s == main) << '\n';
            std::cout << "PStack size: " << pState.getStackSize() << '\n';
            
            return 0;
        },
        state.get());
    std::cout << "Status: " << static_cast<int>(status) << '\n';
    if (status != lat::LuaStatus::Ok)
        return 0;

    constexpr int toPush = LUA_MINSTACK * 20;
    allocatorData.mStackSize = api.getStackSize();
    std::cout << "Stack size: " << api.getStackSize() << '\n';
    for (int i = 0; i < toPush; ++i)
    {
        api.pushInteger(i);
        allocatorData.mStackSize = api.getStackSize();
    }
    std::cout << "\nStack size: " << api.getStackSize() << '\n';
    for (int i = 1; i <= toPush; ++i)
    {
        std::cout << "Int at " << i << ": " << api.asInteger(-i) << '\n';
    }
}

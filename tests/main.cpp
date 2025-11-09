#include <lua/api.hpp>
#include <stack.hpp>
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
    state.setDebugHook(
        [](lat::Stack& stack, lua_Debug&) {
            std::cout << "Called debug hook\n";
            stack.ensure(LUAI_MAXCSTACK + 5);
        },
        lat::LuaHookMask::Count, 1);
    try
    {
        state.withStack([](lat::Stack& stack) {
            constexpr std::string_view code = "return 1+1";
            stack.pushFunction(code);
            lat::LuaApi api(*stack.get());
            api.call(0, 1);
            lua_Number output = api.asNumber(-1);
            std::cout << code << " = " << output << '\n';
        });
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception " << e.what() << '\n';
    }
}

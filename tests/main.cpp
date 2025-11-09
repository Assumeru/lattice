#include <lua/api.hpp>
#include <stack.hpp>
#include <state.hpp>
#include <object.hpp>

#include <array>
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
    state.setDebugHook([](lat::Stack&, lua_Debug&) { std::cout << "Called debug hook\n"; }, lat::LuaHookMask::Count, 2);
    try
    {
        constexpr std::array<lat::Library, 1> toLoad{ lat::Library::String };
        state.loadLibraries(toLoad);
        state.withStack([](lat::Stack& stack) {
            constexpr std::string_view code = "return { a = 2, [2] = 'c' }";
            stack.pushFunction(code);
            lat::LuaApi api(*stack.get());
            api.call(0, 1);
            auto object = stack.getObject(-1);
            std::cout << "a = " << object["a"].as<int>() << '\n';
            std::cout << "2 = " << object[object["a"]].as<std::string_view>() << '\n';
        });
    }
    catch (const std::exception& e)
    {
        std::cout << "Exception " << e.what() << '\n';
    }
}

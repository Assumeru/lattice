#include <lua/api.hpp>
#include <object.hpp>
#include <stack.hpp>
#include <state.hpp>

#include <array>
#include <iostream>

#include <gtest/gtest.h>

namespace
{
    void test()
    {
        lat::State state;
        try
        {
            constexpr std::array<lat::Library, 1> toLoad{ lat::Library::String };
            std::cout << "load libs\n";
            state.loadLibraries(toLoad);
            state.withStack([](lat::Stack& stack) {
                std::cout << "start\n";
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
}

int main(int argc, char** argv)
{
    test();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

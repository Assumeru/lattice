#include <function.hpp>
#include <lua/api.hpp>
#include <object.hpp>
#include <stack.hpp>
#include <state.hpp>
#include <table.hpp>

#include <iostream>

#include <gtest/gtest.h>

namespace
{
    void test()
    {
        lat::State state;
        try
        {
            state.withStack([](lat::Stack& stack) {
                std::cout << "start\n";
                lat::LuaApi api(*stack.get());
                api.pushFunction([](lua_State* state) -> int {
                    std::cout << "stack size inside: " << lua_gettop(state) << "\n";
                    return 0;
                });
                lat::FunctionView function = stack.getObject(-1).asFunction();
                std::cout << "stack size outside: " << api.getStackSize() << "\n";
                function("a", 1, 2);
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

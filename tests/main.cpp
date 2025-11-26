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
    struct Struct
    {
    };
    void test()
    {
        lat::State state;
        try
        {
            state.loadLibraries({});
            state.withStack([&](lat::Stack& stack) {
                std::cout << "start\n";
                auto function = stack.pushFunction([](int a, int b) {
                    std::cout << "test: " << a << b << '\n';
                    return a + b;
                });
                stack["f"] = function;
                stack.execute(R"(
                    local function test(...)
                        local args = { ... }
                        local status, res = pcall(function() return f(unpack(args)) end)
                        if status then
                            print('success', res)
                        else
                            print('error', res)
                        end
                    end
                    test(1)
                    test(1, 2)
                    )");
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

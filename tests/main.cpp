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
                    if (b == 4)
                        throw std::runtime_error("b == 4");
                    return a + b;
                });
                stack["f"] = function;
                stack.execute(R"(
                    local function test(...)
                        local args = { ... }
                        local function test2() return f(unpack(args)) end
                        local status, res = pcall(test2)
                        if status then
                            print('success', res)
                        else
                            print('error', res)
                        end
                    end
                    test(1)
                    test(1, 'a')
                    test(1, 2)
                    test(1, 4)
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

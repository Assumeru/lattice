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
                int a = 123;
                stack.pushLightUserData(&api);
                const int apiPtr = stack.makeAbsolute(-1);
                stack.pushLightUserData(&a);
                const int intPtr = stack.makeAbsolute(-1);
                std::cout << "light size: " << api.getObjectSize(apiPtr) << '\n';
                if (api.pushMetatable(apiPtr))
                    std::cout << "unexpected metatable 1\n";
                if (api.pushMetatable(intPtr))
                    std::cout << "unexpected metatable 2\n";
                stack.pushTable();
                api.setMetatable(intPtr);
                if (!api.pushMetatable(intPtr))
                    throw std::runtime_error("failed to set metatable");
                if (!api.pushMetatable(apiPtr))
                    api.pushNil();
                std::cout << "equal metatables == " << api.equal(-1, -2) << "\n";
                stack.pop(2);
                stack.pushTable();
                if (!api.setEnvTable(intPtr))
                    std::cout << "failed to set env table 1\n";
                api.pushEnvTable(intPtr);
                api.pushEnvTable(apiPtr);
                std::cout << "equal envs == " << api.equal(-1, -2) << "\n";
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

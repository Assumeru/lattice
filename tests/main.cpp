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
        int mValue;
    };
    void test()
    {
        lat::State state;
        try
        {
            state.loadLibraries();
            state.withStack([&](lat::Stack& stack) {
                std::cout << "start\n";
                auto type = stack.newUserType<Struct>("Struct");
                type["value2"] = 1;
                type.setProperty("value", [](const Struct& s) { return s.mValue; }, [](Struct& s, int v) { s.mValue = v; });
                type[lat::meta::toString] = [](const Struct& s) { return "struct " + std::to_string(s.mValue); };
                stack["v"] = Struct{ 2 };
                stack.execute(R"(
                    print(v, v.value, v.value2)
                    v.value = 3
                    print(v.value)
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

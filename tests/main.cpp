#include <lua/api.hpp>
#include <object.hpp>
#include <stack.hpp>
#include <state.hpp>
#include <table.hpp>

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
                // std::cout << "string == nil " << static_cast<int>(stack["string"].get().getType()) << '\n';
                constexpr std::string_view code = "return { a = 2, [2] = 'c', b = { c = 'd' } }";
                stack.pushFunction(code);
                lat::LuaApi api(*stack.get());
                api.call(0, 1);
                auto object = stack.getObject(-1).asTable();
                lat::ObjectView a = object["a"];
                // lat::ObjectView a = object.get("a");
                std::cout << "a = " << a.as<int>() << '\n';
                lat::ObjectView c = object[a];
                // lat::ObjectView c = object.get(a);
                std::cout << "c = " << c.as<std::string_view>() << '\n';
                lat::ObjectView bc = object["b"][c];
                // lat::ObjectView bc = object.get("b", "c");
                std::cout << "b.c = " << bc.as<std::string_view>() << '\n';
                // object.set(1, "b", "d");
                object["b"]["d"] = 1;
                // lat::ObjectView bd = object.get("b", "d");
                lat::ObjectView bd = object["b"]["d"];
                std::cout << "b.d = " << bd.as<int>() << '\n';
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

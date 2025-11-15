#include <function.hpp>
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
                lat::FunctionView f = stack.pushFunction(code);
                std::cout << "size: " << stack.getTop() << '\n';
                lat::TableView object = f.invoke<lat::TableView>();
                std::cout << "size: " << stack.getTop() << '\n';
                // std::cout << "size: " << stack.getTop() << ", type: " << static_cast<int>(r.getType()) << '\n';
                // lat::TableView object = stack.getObject(-1).asTable();
                lat::ObjectView a = object["a"];
                // lat::ObjectView a = object.get("a");
                std::cout << "a = " << a.as<int>() << '\n';
                lat::ObjectView c = object[a];
                // lat::ObjectView c = object.get(a);
                std::cout << "c = " << static_cast<int>( c.getType()) << c.as<std::string_view>() << '\n';
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

#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace
{
    using namespace lat;
    using namespace std::literals;

    constexpr std::array libTables{ "package"sv, "string"sv, "table"sv, "math"sv, "io"sv, "os"sv, "debug"sv };

    struct LibraryTest : public testing::Test
    {
        State mState;

        void checkNoLibs()
        {
            mState.withStack([](Stack& stack) {
                ObjectView globals = stack["_G"];
                EXPECT_EQ(globals, nil);
                stack.pop();

                for (std::string_view key : libTables)
                {
                    ObjectView lib = stack[key];
                    EXPECT_EQ(lib, nil);
                    stack.pop();
                }
            });
        }

        void checkAllLibs()
        {
            mState.withStack([](Stack& stack) {
                ObjectView globals = stack["_G"];
                EXPECT_TRUE(globals.isTable());
                stack.pop();

                for (std::string_view key : libTables)
                {
                    ObjectView lib = stack[key];
                    EXPECT_TRUE(lib.isTable());
                    stack.pop();
                }
            });
        }
    };

    TEST_F(LibraryTest, load_libraries_without_args_loads_all)
    {
        checkNoLibs();
        mState.loadLibraries();
        checkAllLibs();
    }

    TEST_F(LibraryTest, load_all)
    {
        checkNoLibs();
        mState.loadLibraries({ { Library::Base, Library::Package, Library::String, Library::Table, Library::Math,
            Library::IO, Library::OS, Library::Debug } });
        checkAllLibs();
    }
}

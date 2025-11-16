#include <function.hpp>
#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    struct FunctionTest : public testing::Test
    {
        State mState;
    };

    TEST_F(FunctionTest, can_call_library_function)
    {
        mState.loadLibraries({ { Library::Math } });
        mState.withStack([](Stack& stack) {
            FunctionView min = stack["math"]["min"];
            int result = min.invoke<int>(3, 2, 1);
            EXPECT_EQ(result, 1);
        });
    }
}

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

    TEST_F(FunctionTest, can_call_interpreted_function)
    {
        mState.withStack([](Stack& stack) {
            ReturningFunctionView<int> function = stack.pushFunctionReturning<int>("return 2");
            EXPECT_EQ(function(), 2);
        });
    }

    TEST_F(FunctionTest, can_generate_functions)
    {
        mState.withStack([](Stack& stack) {
            auto generator = stack.pushFunctionReturning<ReturningFunctionView<int>>("return function(a) return a end");
            auto function = generator();
            EXPECT_EQ(function(3), 3);
        });
    }

    TEST_F(FunctionTest, can_return_multiple_values)
    {
        mState.withStack([](Stack& stack) {
            auto function = stack.pushFunctionReturning<int, int, int>("return 1, 2, 3");
            auto [first, second, third] = function();
            EXPECT_EQ(first, 1);
            EXPECT_EQ(second, 2);
            EXPECT_EQ(third, 3);
            FunctionView raw = function;
            EXPECT_EQ(raw.invoke<int>(), 1);
        });
    }

    TEST_F(FunctionTest, can_call_coroutines)
    {
        mState.loadLibraries({ { Library::Base } });
        mState.withStack([](Stack& stack) {
            auto generator = stack.pushFunctionReturning<ObjectView>("return coroutine.create(function(a) a = coroutine.yield(a, 1) coroutine.yield(a, 2) end)");
            auto coroutine = generator();
            auto script = stack.pushFunction("return function(t) return function(a) return coroutine.resume(t, a) end end");
            auto resumeGenerator = script.invoke<FunctionView>();
            auto resume = resumeGenerator.invoke<ReturningFunctionView<std::tuple<bool, int, int>>>(coroutine);
            {
                auto [success, input, value] = resume(11);
                EXPECT_TRUE(success);
                EXPECT_EQ(input, 11);
                EXPECT_EQ(value, 1);
            }
            {
                auto [success, input, value] = resume(-3);
                EXPECT_TRUE(success);
                EXPECT_EQ(input, -3);
                EXPECT_EQ(value, 2);
            }
            EXPECT_TRUE(resume.invoke<bool>());
            EXPECT_FALSE(resume.invoke<bool>());
        });
    }
}

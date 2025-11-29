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
            auto function = stack.execute<ReturningFunctionView<int>>("return function(a) return a end");
            EXPECT_EQ(function(3), 3);
        });
    }

    TEST_F(FunctionTest, arguments_are_passed_in_the_right_order)
    {
        mState.loadLibraries({ { Library::Base } });
        mState.withStack([](Stack& stack) {
            auto function = stack.execute<ReturningFunctionView<int>>(
                "return function(a, b) if (a == true) then error(a) end return b end");
            EXPECT_ANY_THROW(function(true, 1));
            EXPECT_EQ(function(false, 2), 2);
            EXPECT_EQ(function(false, 3), 3);
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
            auto results = raw.invoke<std::vector<ObjectView>>();
            EXPECT_EQ(results[0].asInt(), 1);
            EXPECT_EQ(results[1].asInt(), 2);
            EXPECT_EQ(results[2].asInt(), 3);
        });
    }

    TEST_F(FunctionTest, can_return_variable_values)
    {
        mState.withStack([](Stack& stack) {
            auto function
                = stack.execute<ReturningFunctionView<std::vector<ObjectView>>>("return function(...) return ... end");
            {
                EXPECT_TRUE(function().empty());
            }
            {
                auto results = function(123);
                EXPECT_EQ(results.size(), 1);
                EXPECT_EQ(results[0].asInt(), 123);
            }
            {
                auto results = function("a", "b", 1, 2);
                EXPECT_EQ(results.size(), 4);
                EXPECT_EQ(results[0].asString(), "a");
                EXPECT_EQ(results[1].asString(), "b");
                EXPECT_EQ(results[2].asInt(), 1);
                EXPECT_EQ(results[3].asInt(), 2);
            }
        });
    }

    TEST_F(FunctionTest, can_call_coroutines)
    {
        mState.loadLibraries({ { Library::Base } });
        mState.withStack([](Stack& stack) {
            auto coroutine = stack.execute<ObjectView>(
                "return coroutine.create(function(a) a = coroutine.yield(a, 1) coroutine.yield(a, 2) end)");
            auto resumeGenerator = stack.execute<FunctionView>(
                "return function(t) return function(a) return coroutine.resume(t, a) end end");
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

    TEST_F(FunctionTest, can_call_lambda_from_lua)
    {
        mState.withStack([](Stack& stack) {
            bool called = false;
            stack["f"] = [&] { called = true; };
            stack.execute("f()");
            EXPECT_TRUE(called);
        });
    }

    TEST_F(FunctionTest, can_convert_lua_arguments)
    {
        mState.withStack([](Stack& stack) {
            stack["f"] = [](std::string_view a, int b, float c, TableView d) {
                EXPECT_EQ(a, "a");
                EXPECT_EQ(b, 1);
                EXPECT_EQ(c, 0.2f);
                EXPECT_EQ(d.size(), 0);
            };
            stack.execute("f('a', 1, 0.2, {})");
        });
    }

    TEST_F(FunctionTest, can_use_stack_argument)
    {
        mState.withStack([](Stack& stack) {
            FunctionView function = stack.pushFunction([](Stack& a, Stack& b) { EXPECT_EQ(&a, &b); });
            function();
        });
    }

    TEST_F(FunctionTest, lambda_can_return_values)
    {
        mState.loadLibraries({ { Library::Base } });
        mState.withStack([](Stack& stack) {
            stack["f1"] = [](int a, int b) { return a + b; };
            stack.execute("if f1(1, 2) ~= 3 then error('1 + 2 = 3') end");
            stack["f2"] = [](int a, int b) { return std::tuple(a + 2, b + 1); };
            stack.execute(R"(
                local a, b = f2(2, 3)
                if (a ~= b) then error('4 == 4') end
                )");
        });
    }

    TEST_F(FunctionTest, can_overload_functions)
    {
        mState.loadLibraries({ { Library::Base } });
        mState.withStack([](Stack& stack) {
            stack["f"] = Overload([](int a) { return a + 1; }, [](Nil) { return -1; },
                [](Stack&, std::string_view value) { return value[0]; }, [x = 0]() mutable { return ++x; });
            stack.execute(R"(
                local function test(result, expected)
                    if result ~= expected then error('Expected ' .. expected .. ' got ' .. result) end
                end
                test(f(1), 2)
                test(f(2), 3)
                test(f(nil), -1)
                test(f(), 1)
                test(f(), 2)
                test(f('a'), 97)
                test(f('b'), 98)
                )");
        });
    }
}

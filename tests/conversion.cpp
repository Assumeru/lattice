#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    struct ConversionTest : public testing::Test
    {
        State mState;
    };

    TEST_F(ConversionTest, can_push_nil_constant)
    {
        mState.withStack([](Stack& stack) {
            ObjectView view = stack.push(nil);
            EXPECT_TRUE(view.isNil());
            EXPECT_TRUE(view.is<Nil>());
        });
    }

    enum class Enum
    {
        Test
    };

    TEST_F(ConversionTest, can_push_enum_class)
    {
        mState.withStack([](Stack& stack) {
            ObjectView view = stack.push(Enum::Test);
            EXPECT_EQ(view.asInt(), static_cast<std::ptrdiff_t>(Enum::Test));
            EXPECT_EQ(view.asInt(), view.as<std::ptrdiff_t>());
        });
    }

    TEST_F(ConversionTest, can_push_string_literal)
    {
        mState.withStack([](Stack& stack) {
            ObjectView view = stack.push("abc");
            EXPECT_EQ(view.asString(), "abc");
            EXPECT_EQ(view.as<std::string_view>(), "abc");
            EXPECT_EQ(view.as<std::string>(), "abc");
        });
    }

    TEST_F(ConversionTest, int_ref_decays_to_int)
    {
        mState.withStack([](Stack& stack) {
            const int value = 456;
            ObjectView view = stack.push(value);
            EXPECT_EQ(view.as<int>(), value);
        });
    }

    TEST_F(ConversionTest, can_convert_bool)
    {
        mState.withStack([](Stack& stack) {
            ObjectView view = stack.push(true);
            EXPECT_EQ(view.as<bool>(), true);
            EXPECT_EQ(view.asBool(), true);
        });
    }

    TEST_F(ConversionTest, can_convert_float)
    {
        mState.withStack([](Stack& stack) {
            float value = 1.2f;
            ObjectView view = stack.push(value);
            EXPECT_EQ(view.as<float>(), value);
            EXPECT_EQ(view.as<double>(), value);
            EXPECT_EQ(view.as<int>(), 1);
            EXPECT_TRUE(view.is<int>());
            EXPECT_TRUE(view.is<float>());
            EXPECT_TRUE(view.is<double>());
        });
    }

    TEST_F(ConversionTest, int_is_not_string)
    {
        mState.withStack([](Stack& stack) {
            int value = 1;
            ObjectView view = stack.push(value);
            EXPECT_FALSE(stack.isString(1));
            EXPECT_TRUE(view.is<int>());
            EXPECT_FALSE(view.is<std::string_view>());
            EXPECT_EQ(view.as<int>(), 1);
        });
    }

    TEST_F(ConversionTest, can_push_and_pull_variants)
    {
        using V = std::variant<std::string_view, int>;
        mState.withStack([](Stack& stack) {
            {
                V value = 1;
                lat::ObjectView view = stack.push(value);
                EXPECT_TRUE(view.isNumber());
                EXPECT_FALSE(view.isString());
                EXPECT_TRUE(view.is<V>());
                stack.pop();
            }
            {
                V value = "a";
                lat::ObjectView view = stack.push(value);
                EXPECT_FALSE(view.isNumber());
                EXPECT_TRUE(view.isString());
                EXPECT_TRUE(view.is<V>());
                stack.pop();
            }
            lat::FunctionView function = stack.pushFunction([](bool string) -> V {
                if (string)
                    return "abc";
                return 123;
            });
            EXPECT_EQ(1, stack.getTop());
            EXPECT_EQ("abc", function.invoke<std::string_view>(true));
            EXPECT_EQ(123, function.invoke<int>(false));
            static_assert(detail::PullSpecialized<V>);
            {
                V value = function.invoke<V>(true);
                EXPECT_EQ(std::get<std::string_view>(value), "abc");
            }
            {
                V value = function.invoke<V>(false);
                EXPECT_EQ(std::get<int>(value), 123);
            }
            EXPECT_EQ(1, stack.getTop());
        });
    }
}

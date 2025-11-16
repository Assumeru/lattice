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
}

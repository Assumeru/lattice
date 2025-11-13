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
            pushToStack(stack, nil);
            EXPECT_TRUE(stack.isNil(-1));
        });
    }

    enum class Enum
    {
        Test
    };

    TEST_F(ConversionTest, can_push_enum_class)
    {
        mState.withStack([](Stack& stack) {
            pushToStack(stack, Enum::Test);
            std::ptrdiff_t pushed = stack.getObject(-1).asInt();
            EXPECT_EQ(pushed, static_cast<std::ptrdiff_t>(Enum::Test));
        });
    }

    TEST_F(ConversionTest, can_push_string_literal)
    {
        mState.withStack([](Stack& stack) {
            pushToStack(stack, "abc");
            std::string_view pushed = stack.getObject(-1).asString();
            EXPECT_EQ(pushed, "abc");
        });
    }

    TEST_F(ConversionTest, int_ref_decays_to_int)
    {
        mState.withStack([](Stack& stack) {
            const int value = 456;
            pushToStack(stack, value);
            std::ptrdiff_t pushed = stack.getObject(-1).asInt();
            EXPECT_EQ(pushed, value);
        });
    }
}

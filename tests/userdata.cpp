#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    struct UserDataTest : public testing::Test
    {
        State mState;
    };

    struct TestData
    {
        int mValue;
    };

    TEST_F(UserDataTest, can_push_pointer_as_userdata)
    {
        mState.withStack([](Stack& stack) {
            const TestData value{ 2 };
            const TestData* pointer = &value;
            ObjectView view = stack.push(pointer);
            EXPECT_TRUE(view.isUserData());
            std::span<std::byte> data = view.asUserData();
            EXPECT_EQ(sizeof(TestData*), data.size());
            EXPECT_EQ(std::memcmp(data.data(), &pointer, data.size()), 0);
        });
    }

    TEST_F(UserDataTest, can_push_value_as_userdata)
    {
        mState.withStack([](Stack& stack) {
            const TestData value{ 3 };
            ObjectView view = stack.push(value);
            EXPECT_TRUE(view.isUserData());
            std::span<std::byte> data = view.asUserData();
            EXPECT_LT(sizeof(TestData*), data.size());
            void* pointer = nullptr;
            std::memcpy(&pointer, data.data(), sizeof(void*));
            EXPECT_NE(pointer, nullptr);
            EXPECT_EQ(static_cast<TestData*>(pointer)->mValue, value.mValue);
        });
    }

    struct DestructorTestData
    {
        bool* mDestroyed;

        DestructorTestData(bool& destroyed)
            : mDestroyed(&destroyed)
        {
        }

        ~DestructorTestData() { *mDestroyed = true; }
    };

    TEST_F(UserDataTest, pointer_userdata_does_not_get_destroyed)
    {
        mState.withStack([](Stack& stack) {
            bool destroyed = false;
            DestructorTestData value(destroyed);
            stack.push(&value);
            stack.pop();
            EXPECT_EQ(stack.getTop(), 0);
            stack.collectGarbage();
            EXPECT_FALSE(destroyed);
        });
    }

    TEST_F(UserDataTest, value_userdata_calls_destructor)
    {
        mState.withStack([](Stack& stack) {
            bool destroyed = false;
            DestructorTestData value(destroyed);
            stack.push(value);
            stack.pop();
            EXPECT_EQ(stack.getTop(), 0);
            stack.collectGarbage();
            EXPECT_TRUE(destroyed);
        });
    }
}

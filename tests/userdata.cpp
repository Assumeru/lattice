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

    constexpr std::size_t pointerSize = sizeof(void*);

    void* extractPointer(std::span<std::byte> data)
    {
        void* pointer = nullptr;
        std::memcpy(&pointer, data.data(), pointerSize);
        return pointer;
    }

    TEST_F(UserDataTest, can_push_pointer_as_userdata)
    {
        mState.withStack([](Stack& stack) {
            const TestData value{ 2 };
            const TestData* pointer = &value;
            ObjectView view = stack.push(pointer);
            EXPECT_TRUE(view.isUserData());
            std::span<std::byte> data = view.asUserData();
            EXPECT_EQ(pointerSize, data.size());
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
            EXPECT_LT(pointerSize, data.size());
            void* pointer = extractPointer(data);
            EXPECT_NE(pointer, nullptr);
            EXPECT_EQ(static_cast<TestData*>(pointer)->mValue, value.mValue);
        });
    }

    struct DestructorTestData
    {
        bool* mDestroyed;

        explicit DestructorTestData(bool& destroyed)
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
            EXPECT_FALSE(destroyed);
            stack.pop();
            EXPECT_EQ(stack.getTop(), 0);
            stack.collectGarbage();
            EXPECT_TRUE(destroyed);
        });
    }

    struct MoveOnlyTestData
    {
        int mValue;

        explicit MoveOnlyTestData(int value)
            : mValue(value)
        {
        }

        MoveOnlyTestData(const MoveOnlyTestData&) = delete;

        MoveOnlyTestData(MoveOnlyTestData&& value)
            : mValue(value.mValue + 1)
        {
        }
    };

    TEST_F(UserDataTest, move_only_types_can_be_userdata)
    {
        mState.withStack([](Stack& stack) {
            ObjectView view = stack.push(MoveOnlyTestData(1));
            std::span<std::byte> data = view.asUserData();
            void* pointer = extractPointer(data);
            EXPECT_GE(static_cast<MoveOnlyTestData*>(pointer)->mValue, 1);
        });
    }
}

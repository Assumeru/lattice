#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

#include <cstring>

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
            EXPECT_EQ(view.as<const TestData*>()->mValue, value.mValue);
        });
    }

    TEST_F(UserDataTest, can_push_reference_wrapper)
    {
        mState.withStack([](Stack& stack) {
            TestData value{ 4 };
            std::reference_wrapper<TestData> wrapper = value;
            ObjectView view = stack.push(wrapper);
            EXPECT_TRUE(view.isUserData());
            std::span<std::byte> data = view.asUserData();
            EXPECT_EQ(pointerSize, data.size());
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

    TEST_F(UserDataTest, can_get_userdata_value)
    {
        mState.withStack([](Stack& stack) {
            TestData value{ 4 };
            ObjectView view = stack.push(value);
            {
                TestData* pointer = view.as<TestData*>();
                EXPECT_EQ(pointer->mValue++, value.mValue++);
            }
            {
                const TestData* pointer = view.as<const TestData*>();
                EXPECT_EQ(pointer->mValue, value.mValue);
            }
            {
                TestData& ref = view.as<TestData&>();
                EXPECT_EQ(ref.mValue++, value.mValue++);
            }
            {
                const TestData& ref = view.as<const TestData&>();
                EXPECT_EQ(ref.mValue, value.mValue);
            }
            {
                const auto ref = view.as<std::reference_wrapper<TestData>>();
                EXPECT_EQ(ref.get().mValue, value.mValue);
            }
            EXPECT_ANY_THROW(view.as<const DestructorTestData*>());
            {
                TestData copy = view.as<TestData>();
                EXPECT_EQ(copy.mValue, value.mValue);
            }
        });
    }

    struct MoveOnlyTestData
    {
        int mValue;
        bool mMoved = false;

        explicit MoveOnlyTestData(int value)
            : mValue(value)
        {
        }

        MoveOnlyTestData(const MoveOnlyTestData&) = delete;

        MoveOnlyTestData(MoveOnlyTestData&& value)
            : mValue(value.mValue + 1)
        {
            value.mMoved = true;
        }
    };

    TEST_F(UserDataTest, move_only_types_can_be_userdata)
    {
        mState.withStack([](Stack& stack) {
            ObjectView view = stack.push(MoveOnlyTestData(1));
            const MoveOnlyTestData* pointer = view.as<const MoveOnlyTestData*>();
            EXPECT_EQ(pointer->mValue, 2);
            EXPECT_FALSE(pointer->mMoved);
            MoveOnlyTestData data = view.as<MoveOnlyTestData>();
            EXPECT_EQ(data.mValue, 3);
            EXPECT_FALSE(data.mMoved);
            EXPECT_TRUE(pointer->mMoved);
        });
    }
}

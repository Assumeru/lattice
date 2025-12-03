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

    TEST_F(UserDataTest, can_define_bindings)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            type.setProperty(
                "value", [](const TestData& data) { return data.mValue; },
                [](TestData& data, int v) { data.mValue = v; });
            EXPECT_EQ(stack.getTop(), 0);
            TestData data{ 2 };
            stack["v"] = &data;
            stack.execute("v.value = v.value + 1");
            EXPECT_EQ(data.mValue, 3);
        });
    }

    TEST_F(UserDataTest, cannot_redefine_usertype)
    {
        mState.withStack([](Stack& stack) {
            stack.newUserType<TestData>("TestData");
            EXPECT_ANY_THROW(stack.newUserType<TestData>("DefinitelyNotTestData"));
        });
    }

    TEST_F(UserDataTest, can_define_readonly_property)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            type.setReadOnlyProperty("value", [](const TestData& data) { return data.mValue; });
            TestData data{ 2 };
            stack["v"] = &data;
            auto f = stack.pushFunctionReturning<int>("return v.value");
            EXPECT_EQ(f(), data.mValue);
            EXPECT_ANY_THROW(stack.execute("v.value = 1"));
        });
    }

    TEST_F(UserDataTest, can_define_writeonly_property)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            type.setWriteOnlyProperty("value", [](TestData& data, int v) { data.mValue = v; });
            TestData data{ 2 };
            stack["v"] = &data;
            auto f = stack.pushFunction("v.value = ...");
            EXPECT_EQ(data.mValue, 2);
            f(1);
            EXPECT_EQ(data.mValue, 1);
            EXPECT_ANY_THROW(stack.execute("local a = v.value"));
        });
    }

    TEST_F(UserDataTest, can_define_property)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            type["value"] = 2;
            TestData data{ 3 };
            stack["v"] = &data;
            auto f = stack.pushFunctionReturning<int>("return v.value");
            EXPECT_EQ(f(), 2);
        });
    }

    TEST_F(UserDataTest, can_override_newindex)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            std::map<std::string, int> values;
            type[meta::newIndex] = [&](TestData&, std::string_view key, int value) { values.emplace(key, value); };
            stack["v"] = TestData{ 1 };
            stack.execute("v.a = 1; v.b = 2");
            EXPECT_EQ(values["a"], 1);
            EXPECT_EQ(values["b"], 2);
        });
    }

    TEST_F(UserDataTest, can_override_math)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            type[meta::add] = [](const TestData& d, int v) { return d.mValue + v * 2; };
            type[meta::sub] = [](const TestData& d, int v) { return d.mValue - v * 2; };
            type[meta::mul] = [](const TestData& d, int v) { return d.mValue * v; };
            type[meta::div] = [](const TestData& d, int v) { return d.mValue / v; };
            type[meta::mod] = []() { return 42; };
            type[meta::pow] = []() { return 4; };
            type[meta::unm] = []() { return -1; };
            TestData data(2);
            stack["v"] = &data;
            {
                auto f = stack.pushFunctionReturning<int>("return v + 3");
                EXPECT_EQ(f(), data.mValue + 6);
                stack.pop();
            }
            {
                auto f = stack.pushFunctionReturning<int>("return v - 4");
                EXPECT_EQ(f(), data.mValue - 8);
                stack.pop();
            }
            {
                auto f = stack.pushFunctionReturning<int>("return v * 5");
                EXPECT_EQ(f(), data.mValue * 5);
                stack.pop();
            }
            {
                auto f = stack.pushFunctionReturning<int>("return v / 2");
                EXPECT_EQ(f(), data.mValue / 2);
                stack.pop();
            }
            {
                auto f = stack.pushFunctionReturning<int>("return v % 2");
                EXPECT_EQ(f(), 42);
                stack.pop();
            }
            {
                auto f = stack.pushFunctionReturning<int>("return v ^ 3");
                EXPECT_EQ(f(), 4);
                stack.pop();
            }
            {
                auto f = stack.pushFunctionReturning<int>("return -v");
                EXPECT_EQ(f(), -1);
                stack.pop();
            }
            EXPECT_EQ(stack.getTop(), 0);
        });
    }

    struct DerivedTestData : public TestData
    {
        explicit DerivedTestData(int v)
            : TestData{ v }
        {
        }

        virtual int foo() const { return 4; }
    };

    struct DerivedDerivedTestData : public DerivedTestData
    {
        explicit DerivedDerivedTestData(int v)
            : DerivedTestData(v)
        {
        }

        int foo() const override { return 5; }
    };

    TEST_F(UserDataTest, can_use_base_types)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<DerivedDerivedTestData, DerivedTestData, TestData>("DerivedDerivedTestData");
            type.setReadOnlyProperty("ddvalue", [](const DerivedDerivedTestData& d) { return d.mValue; });
            type.setReadOnlyProperty("dvalue", [](const DerivedTestData& d) { return d.mValue; });
            type.setReadOnlyProperty("bvalue", [](const TestData& d) { return d.mValue; });
            type.setReadOnlyProperty("foo1", [](const DerivedDerivedTestData& d) { return d.foo(); });
            type.setReadOnlyProperty("foo2", [](const DerivedTestData& d) { return d.foo(); });
            DerivedDerivedTestData data(12);
            stack["v"] = &data;
            EXPECT_EQ(stack.getTop(), 0);
            auto f = stack.pushFunctionReturning<int, int, int, int, int>(
                "return v.ddvalue, v.dvalue, v.bvalue, v.foo1, v.foo2");
            const auto [ddvalue, dvalue, bvalue, foo1, foo2] = f();
            EXPECT_EQ(ddvalue, data.mValue);
            EXPECT_EQ(dvalue, data.mValue);
            EXPECT_EQ(bvalue, data.mValue);
            EXPECT_EQ(foo1, data.foo());
            EXPECT_EQ(foo2, data.foo());
        });
    }

    struct MultipleInheritanceData
    {
        virtual int foo() const { return 3; }
    };

    struct MultipleInheritanceTestData : public MultipleInheritanceData, public TestData
    {
        explicit MultipleInheritanceTestData(int v)
            : MultipleInheritanceData()
            , TestData{ v }
        {
        }
    };

    TEST_F(UserDataTest, can_use_multiple_inheritance)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<MultipleInheritanceTestData, MultipleInheritanceData, TestData>(
                "MultipleInheritanceTestData");
            type.setReadOnlyProperty("mvalue", [](const MultipleInheritanceTestData& d) { return d.mValue; });
            type.setReadOnlyProperty("bvalue", [](const TestData& d) { return d.mValue; });
            type.setReadOnlyProperty("foo1", [](const MultipleInheritanceData& d) { return d.foo(); });
            type.setReadOnlyProperty("foo2", [](const MultipleInheritanceTestData& d) { return d.foo(); });
            MultipleInheritanceTestData data(2);
            stack["v"] = &data;
            EXPECT_EQ(stack.getTop(), 0);
            auto f = stack.pushFunctionReturning<int, int, int, int>("return v.mvalue, v.bvalue, v.foo1, v.foo2");
            const auto [mvalue, bvalue, foo1, foo2] = f();
            EXPECT_EQ(mvalue, data.mValue);
            EXPECT_EQ(bvalue, data.mValue);
            EXPECT_EQ(foo1, data.foo());
            EXPECT_EQ(foo2, data.foo());
        });
    }
}

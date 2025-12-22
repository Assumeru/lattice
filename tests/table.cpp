#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    struct TableTest : public testing::Test
    {
        State mState;
    };

    TEST_F(TableTest, can_manipulate_table)
    {
        mState.withStack([](Stack& stack) {
            int top = stack.getTop();
            TableView table = stack.pushTable();
            ++top;
            EXPECT_EQ(top, stack.getTop());
            EXPECT_EQ(0, table.size());
            table["a"] = 1;
            table[1] = "b";
            EXPECT_EQ(1, table.size());
            EXPECT_EQ(top, stack.getTop());
            ObjectView entry = table[table["a"]];
            ++top;
            EXPECT_EQ(top, stack.getTop());
            EXPECT_EQ(entry.asString(), "b");
        });
    }

    TEST_F(TableTest, can_nest_tables)
    {
        mState.withStack([](Stack& stack) {
            TableView t1 = stack.pushTable();
            TableView t2 = stack.pushTable();
            t1["a"] = t2;
            t2["b"] = 1;
            EXPECT_EQ(1, t1["a"]["b"].get().asInt());
        });
    }

    TEST_F(TableTest, indexing_into_a_non_table_throws)
    {
        mState.withStack([](Stack& stack) {
            TableView table = stack.pushTable();
            int top = stack.getTop();
            EXPECT_ANY_THROW(table["a"]["b"].get());
            EXPECT_EQ(top, stack.getTop());
        });
    }

    TEST_F(TableTest, can_get_nested_optional)
    {
        mState.withStack([](Stack& stack) {
            TableView t1 = stack.pushTable();
            TableView t2 = stack.pushTable();
            t1["a"] = t2;
            t2["b"] = 1;
            EXPECT_EQ(stack.getTop(), 2);
            {
                auto value = t1.get<std::optional<int>>("a", "b");
                EXPECT_EQ(1, value);
            }
            {
                auto value = t1.get<std::optional<std::string_view>>("a", "b");
                EXPECT_FALSE(value.has_value());
            }
            EXPECT_EQ(stack.getTop(), 2);
        });
    }

    struct DoublePush
    {
    };

    void pushValue(Stack& stack, DoublePush)
    {
        stack.pushNil();
        stack.pushNil();
    }

    TEST_F(TableTest, cannot_use_types_that_push_multiple_values)
    {
        mState.withStack([](Stack& stack) {
            TableView table = stack.pushTable();
            int top = stack.getTop();
            EXPECT_ANY_THROW(table["a"] = DoublePush());
            EXPECT_EQ(top, stack.getTop());
            EXPECT_ANY_THROW(table[DoublePush()] = 1);
            EXPECT_EQ(top, stack.getTop());
        });
    }

    TEST_F(TableTest, can_iterate_table)
    {
        mState.withStack([](Stack& stack) {
            TableView table = stack.pushTable();
            table[1] = 2;
            table["a"] = 3;
            table[3] = 4;
            EXPECT_EQ(stack.getTop(), 1);
            int count = 0;
            for (const auto& [key, value] : table)
            {
                ObjectView keyView = key.pushTo(stack);
                ObjectView valueView = value.pushTo(stack);
                int v = valueView.as<int>();
                if (auto k = keyView.as<std::optional<int>>())
                {
                    if (*k == 1)
                        EXPECT_EQ(v, 2);
                    else
                    {
                        EXPECT_EQ(v, 4);
                        EXPECT_EQ(k, 3);
                    }
                }
                else
                {
                    EXPECT_EQ(keyView.asString(), "a");
                    EXPECT_EQ(v, 3);
                }
                stack.pop(2);
                ++count;
            }
            EXPECT_EQ(count, 3);
            EXPECT_EQ(stack.getTop(), 1);
        });
    }

    TEST_F(TableTest, can_traverse_get)
    {
        mState.withStack([](Stack& stack) {
            TableView t1 = stack.pushTable();
            TableView t2 = stack.pushTable();
            t1["a"] = t2;
            t2["b"] = 1;
            t1["c"] = 2;
            EXPECT_EQ(stack.getTop(), 2);
            {
                auto value = t1.traverseGet<int>("a", "b");
                EXPECT_EQ(1, value);
            }
            {
                auto value = t2.traverseGet<int>("b");
                EXPECT_EQ(1, value);
            }
            {
                auto value = t1.traverseGet<std::string_view>("a", "b");
                EXPECT_FALSE(value.has_value());
            }
            {
                auto value = t1.traverseGet<int>("c", "d");
                EXPECT_FALSE(value.has_value());
            }
            {
                auto value = t1.traverseGet<int>("a", "c");
                EXPECT_FALSE(value.has_value());
            }
            EXPECT_EQ(stack.getTop(), 2);
            stack.pop();
            stack.push(1);
            EXPECT_ANY_THROW(t2.traverseGet<int>("b"));
        });
    }

    struct TestData
    {
        int mValue;
    };

    TEST_F(TableTest, can_pull_userdata)
    {
        mState.withStack([](Stack& stack) {
            TableView table = stack.pushTable();
            table["a"] = TestData(2);
            TestData a = table["a"];
            EXPECT_EQ(a.mValue, 2);
            EXPECT_EQ(stack.getTop(), 1);
        });
    }

    TEST_F(TableTest, can_directly_invoke_function)
    {
        mState.withStack([](Stack& stack) {
            TableView table = stack.pushTable();
            table["a"] = [](int a) { return a + 1; };
            EXPECT_EQ(table["a"].invoke<int>(1), 2);
            EXPECT_EQ(stack.getTop(), 1);
        });
    }

    TEST_F(TableTest, can_index_table_like_usertype)
    {
        mState.withStack([](Stack& stack) {
            auto type = stack.newUserType<TestData>("TestData");
            type[meta::index] = [](const TestData& data, int index) { return data.mValue + index; };
            type[meta::newIndex] = [](TestData& data, int index, int value) { data.mValue = index + value; };
            TestData data(3);
            TableLikeView table = stack.push(&data).asTableLike();
            EXPECT_EQ(table.get<int>(1), 1 + data.mValue);
            EXPECT_EQ(table.get<int>(2), 2 + data.mValue);
            table[1] = 2;
            EXPECT_EQ(data.mValue, 3);
        });
    }

    TEST_F(TableTest, can_index_table_like_userdata)
    {
        mState.withStack([](Stack& stack) {
            TestData data(3);
            ObjectView view = stack.push(&data);
            EXPECT_ANY_THROW(view.asTableLike());
            TableView mt = *view.pushMetatable();
            mt[meta::index] = [](const TestData& data, int index) { return data.mValue + index; };
            TableLikeView table = view.asTableLike();
            EXPECT_EQ(table.get<int>(1), 1 + data.mValue);
            EXPECT_EQ(table.get<int>(2), 2 + data.mValue);
            EXPECT_ANY_THROW(table[3] = 1);
            mt[meta::newIndex] = [](TestData& data, int index, int value) { data.mValue = index + value; };
            table[1] = 2;
            EXPECT_EQ(data.mValue, 3);
            TableView base = stack.pushTable();
            base["a"] = view;
            const int value = base["a"][1];
            EXPECT_EQ(value, 1 + data.mValue);
        });
    }

    TEST_F(TableTest, can_get_table_like_view_size)
    {
        mState.withStack([](Stack& stack) {
            {
                TableView table = stack.pushTable();
                EXPECT_EQ(table.size(), 0);
                table[1] = 1;
                table[2] = 2;
                EXPECT_EQ(table.size(), 2);
                TableLikeView like = table;
                EXPECT_EQ(like.size(), 2);
                stack.pop();
            }
            {
                stack.pushUserData(0);
                ObjectView userdata = stack.getObject(-1);
                TableView mt = stack.pushTable();
                userdata.setMetatable(mt);
                mt[meta::index] = [](ObjectView, int index) { return index; };
                static_assert(detail::GetFromViewSpecialized<TableLikeView>);
                TableLikeView like = userdata.as<TableLikeView>();
                EXPECT_ANY_THROW(like.size());
                mt[meta::len] = [](ObjectView) { return 123; };
                EXPECT_EQ(like.size(), 123);
            }
        });
    }
}

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
}

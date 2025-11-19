#include <lua/api.hpp>
#include <reference.hpp>
#include <stack.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    struct StackTest : public testing::Test
    {
        State mState;
    };

    TEST_F(StackTest, globals_exist)
    {
        mState.withStack([](Stack& stack) {
            ObjectView globals = stack.globals();
            EXPECT_TRUE(globals.isTable());
        });
    }

    TEST_F(StackTest, can_push_and_pop)
    {
        mState.withStack([](Stack& stack) {
            stack.pushBoolean(true);
            stack.pushInteger(2);
            stack.pushNil();
            stack.pushNumber(0.4);
            stack.pushString("5");
            stack.pushTable();
            EXPECT_EQ(stack.getTop(), 6);
            EXPECT_TRUE(stack.isTable(-1));
            EXPECT_TRUE(stack.isString(-2));
            EXPECT_TRUE(stack.isNumber(-3));
            EXPECT_TRUE(stack.isNil(-4));
            EXPECT_TRUE(stack.isNumber(-5));
            EXPECT_TRUE(stack.isBoolean(-6));
            stack.pop();
            EXPECT_EQ(stack.getTop(), 5);
            EXPECT_TRUE(stack.isString(-1));
            stack.pop(5);
            EXPECT_EQ(stack.getTop(), 0);
        });
    }

    TEST_F(StackTest, can_remove)
    {
        mState.withStack([](Stack& stack) {
            stack.pushBoolean(true);
            stack.pushInteger(2);
            stack.pushNil();
            EXPECT_TRUE(stack.isNil(-1));
            EXPECT_TRUE(stack.isNumber(-2));
            EXPECT_TRUE(stack.isBoolean(-3));
            stack.remove(1);
            EXPECT_EQ(stack.getTop(), 2);
            EXPECT_TRUE(stack.isNil(-1));
            EXPECT_TRUE(stack.isNumber(-2));
            stack.remove(2);
            EXPECT_EQ(stack.getTop(), 1);
            EXPECT_TRUE(stack.isNumber(-1));
        });
    }

    TEST_F(StackTest, can_make_index_absolute)
    {
        mState.withStack([](Stack& stack) {
            stack.pushBoolean(true);
            stack.pushInteger(2);
            stack.pushNil();
            EXPECT_EQ(stack.makeAbsolute(-1), stack.getTop());
            EXPECT_EQ(stack.makeAbsolute(-2), 2);
            EXPECT_EQ(stack.makeAbsolute(-3), 1);
            EXPECT_EQ(stack.makeAbsolute(LUA_REGISTRYINDEX), LUA_REGISTRYINDEX);
            EXPECT_EQ(stack.makeAbsolute(LUA_ENVIRONINDEX), LUA_ENVIRONINDEX);
            EXPECT_EQ(stack.makeAbsolute(LUA_GLOBALSINDEX), LUA_GLOBALSINDEX);
        });
    }

    TEST_F(StackTest, can_nest_stacks)
    {
        mState.withStack([&](Stack& stack1) {
            stack1.pushBoolean(true);
            EXPECT_EQ(stack1.getTop(), 1);
            bool nested = false;
            mState.withStack([&](Stack& stack2) {
                nested = true;
                stack2.pushNil();
                EXPECT_EQ(stack2.getTop(), 1);
            });
            EXPECT_TRUE(nested);
            EXPECT_EQ(stack1.getTop(), 1);
        });
    }

    TEST_F(StackTest, cannot_move_between_nested_stacks)
    {
        mState.withStack([&](Stack& stack1) {
            ObjectView object = stack1.pushBoolean(true);
            mState.withStack([&](Stack& stack2) { EXPECT_ANY_THROW(object.pushTo(stack2)); });
            EXPECT_EQ(stack1.getTop(), 1);
        });
    }

    TEST_F(StackTest, can_use_references_across_stacks)
    {
        mState.withStack([&](Stack& stack1) {
            TableReference ref1 = stack1.pushTable().store();
            Reference ref2 = stack1.pushBoolean(true).store();
            stack1.pop(2);
            mState.withStack([&](Stack& stack2) {
                TableView referenced = ref1.pushTo(stack2);
                referenced[1] = 2;
                ref2 = stack2.pushInteger(3);
            });
            int value1 = ref1.pushTo(stack1)[1];
            EXPECT_EQ(value1, 2);
            int value2 = ref2.pushTo(stack1).as<int>();
            EXPECT_EQ(value2, 3);
        });
    }

    TEST_F(StackTest, references_unregister_themselves)
    {
        mState.withStack([&](Stack& stack) {
            TableView registry = stack.getObject(LUA_REGISTRYINDEX).asTable();
            const std::size_t size = registry.size();
            std::optional<TableReference> ref;
            {
                TableView object = stack.pushTable();
                ref.emplace(object.store());
                stack.pop();
            }
            {
                TableView object = ref->pushTo(stack);
                EXPECT_EQ(object.size(), 0);
                stack.pop();
            }
            EXPECT_EQ(size + 1, registry.size());
            ref.reset();
            EXPECT_EQ(size, registry.size());
        });
    }

    TEST_F(StackTest, cannot_push_reference_after_reset)
    {
        mState.withStack([&](Stack& stack) {
            {
                TableView object = stack.pushTable();
                TableReference ref = object.store();
                stack.pop();
                ref.reset();
                EXPECT_ANY_THROW(ref.pushTo(stack));
            }
            {
                ObjectView object = stack.pushNil();
                Reference ref = object.store();
                stack.pop();
                ref.reset();
                EXPECT_ANY_THROW(ref.pushTo(stack));
            }
        });
    }
}

#include <lua/enums.hpp>
#include <state.hpp>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    TEST(DebugHookTest, can_set_and_unset_hook)
    {
        State state;

        int event = -1;
        state.setDebugHook([&](Stack&, lua_Debug& debug) { event = debug.event; }, LuaHookMask::Call);
        state.withStack([](Stack&) {});
        EXPECT_EQ(event, LUA_HOOKCALL);

        event = -1;
        state.disableDebugHook();
        state.withStack([](Stack&) {});
        EXPECT_EQ(event, -1);
    }
}

#include "function.hpp"

#include "lua/api.hpp"

#include <stdexcept>

namespace lat
{
    void FunctionView::cleanUp(int prev)
    {
        const int diff = mStack.getTop() - prev;
        if (diff > 0)
            mStack.pop(static_cast<std::uint16_t>(diff));
    }

    void FunctionView::call(const int prev, int resCount)
    {
        const int argCount = mStack.getTop() - prev;
        if (argCount < 0)
            throw std::runtime_error("expected a positive argument count");
        if (resCount < 0)
            resCount = LUA_MULTRET;
        mStack.protectedCall(argCount, resCount);
        // Lua pushes return values in reverse order, leaving the first return value at the top of the stack. While
        // relatively sensible as a calling convention, it means we can't remove the second return value from the stack
        // without invalidating any views of the first. This code rotates the returned values so we can skip values we
        // want to keep views to while working our way up the stack converting values.
        resCount = mStack.getTop() - prev + 1;
        const int toMove = resCount / 2;
        if (toMove < 1)
            return;
        LuaApi api = mStack.api();
        for (int i = 0; i <= toMove; ++i)
            api.insert(prev + i);
    }
}

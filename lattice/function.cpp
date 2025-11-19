#include "function.hpp"

#include "lua/api.hpp"
#include "reference.hpp"

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
    }

    FunctionReference FunctionView::store()
    {
        return FunctionReference(ObjectView(*this).store());
    }
}

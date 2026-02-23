#include "function.hpp"

#include "lua/api.hpp"
#include "reference.hpp"
#include "table.hpp"

#include <stdexcept>

namespace lat
{
    void FunctionView::cleanUp(int prev) const
    {
        const int diff = mStack.getTop() - prev;
        if (diff > 0)
            mStack.pop(static_cast<std::uint16_t>(diff));
    }

    void FunctionView::call(const int prev, int resCount) const
    {
        const int argCount = mStack.getTop() - prev;
        if (argCount < 0)
            throw std::runtime_error("expected a positive argument count");
        if (resCount < 0)
            resCount = LUA_MULTRET;
        mStack.protectedCall(argCount, resCount);
    }

    FunctionReference FunctionView::store() const
    {
        return FunctionReference(ObjectView(*this).store());
    }

    namespace
    {
        int dumpFunction(lua_State*, const void* buffer, std::size_t size, void* output)
        {
            std::string& byteCode = *static_cast<std::string*>(output);
            byteCode.append(static_cast<const char*>(buffer), size);
            return 0;
        }
    }

    ByteCode FunctionView::dump() const
    {
        ObjectView(*this).pushTo(mStack);
        ByteCode code;
        mStack.api().dumpFunction(&dumpFunction, &code.mCode);
        mStack.pop();
        return code;
    }
}

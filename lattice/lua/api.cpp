#include "api.hpp"

namespace lat
{
    namespace
    {
        std::string getInfoWhere(LuaInfo what, bool function)
        {
            std::string out;
            if (function)
                out += '>';
            if (what & LuaInfo::FillName)
                out += 'n';
            if (what & LuaInfo::FillSource)
                out += 'S';
            if (what & LuaInfo::FillCurrentLine)
                out += 'l';
            if (what & LuaInfo::FillNumUpValues)
                out += 'u';
            if (what & LuaInfo::PushFunction)
                out += 'f';
            if (what & LuaInfo::PushValidLines)
                out += 'L';
            return out;
        }
    }

    bool LuaApi::getInvocationDebugInfo(LuaInfo what, lua_Debug& activationRecord)
    {
        std::string whatString = getInfoWhere(what, false);
        return lua_getinfo(mState, whatString, &activationRecord);
    }

    bool LuaApi::getFunctionDebugInfo(LuaInfo what, lua_Debug& activationRecord)
    {
        std::string whatString = getInfoWhere(what, true);
        return lua_getinfo(mState, whatString, &activationRecord);
    }
}

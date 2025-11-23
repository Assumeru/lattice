#include "userdata.hpp"

#include "lua/api.hpp"
#include "reference.hpp"
#include "stack.hpp"
#include "state.hpp"
#include "table.hpp"

#include <cstring>
#include <stdexcept>

namespace lat
{
    namespace
    {
        int defaultDestructor(lua_State* state)
        {
            LuaApi api(*state);
            api.pushUpValue(1);
            UserDataDestructor destructor = reinterpret_cast<UserDataDestructor>(api.asUserData(-1));
            if (destructor == nullptr)
            {
                api.pushString("missing destructor");
                api.error();
            }
            api.pop(1);
            try
            {
                Stack stack(state);
                destructor(stack, stack.getObject(-1));
                return 0;
            }
            catch (const std::exception& e)
            {
                api.setStackSize(0);
                api.pushString(e.what());
            }
            catch (...)
            {
                api.setStackSize(0);
                api.pushString("error in destructor");
            }
            api.error();
        }
    }

    void UserTypeRegistry::clear()
    {
        mMetatables.clear();
    }

    void UserTypeRegistry::destroyUserData(ObjectView view, Destructor consumer)
    {
        std::span<std::byte> data = view.asUserData();
        if (data.size() < pointerSize)
            throw std::invalid_argument("invalid user data");
        else if (data.size() == pointerSize)
        {
            // "light" user data; pointer only
            return;
        }
        void* pointer = nullptr;
        std::memcpy(&pointer, data.data(), pointerSize);
        if (pointer == nullptr)
            return; // nothing to destroy
        consumer(pointer);
        pointer = nullptr;
        std::memcpy(data.data(), &pointer, pointerSize);
    }

    std::span<std::byte> UserTypeRegistry::pushUserData(
        BasicStack& stack, std::size_t size, const void* pointer, std::type_index type, UserDataDestructor destructor)
    {
        const TableReference& ref = getMetatable(stack, type, destructor);
        TableView metatable = ref.pushTo(stack);
        std::span<std::byte> data = stack.pushUserData(size);
        std::memcpy(data.data(), &pointer, pointerSize);
        stack.getObject(-1).setMetatable(metatable);
        stack.remove(-2);
        return data;
    }

    void UserTypeRegistry::pushUserData(BasicStack& stack, std::size_t size, std::size_t align,
        const std::type_info& type, UserDataDestructor destructor, FunctionRef<void*(void*)> constructor)
    {
        // object + padding to ensure alignment
        const std::size_t dataSize = size + align - 1;
        // Push a user type with the correct metatable. We init to nullptr because construction might throw
        std::span<std::byte> data = pushUserData(stack, pointerSize + dataSize, nullptr, type, destructor);
        void* pointer = data.data() + pointerSize;
        std::size_t alignSize = dataSize;
        pointer = std::align(align, size, pointer, alignSize);
        try
        {
            if (pointer == nullptr)
                throw std::runtime_error(std::string("failed to align object of type ") + type.name());
            pointer = constructor(pointer);
            std::memcpy(data.data(), &pointer, pointerSize);
        }
        catch (...)
        {
            stack.pop();
            throw;
        }
    }

    const TableReference& UserTypeRegistry::getMetatable(
        BasicStack& stack, std::type_index type, UserDataDestructor destructor)
    {
        auto found = mMetatables.find(type);
        if (found == mMetatables.end())
        {
            TableView table = stack.pushTable();
            stack.pushLightUserData(reinterpret_cast<void*>(destructor));
            stack.api().pushFunction(&defaultDestructor, 1);
            table[meta::gc] = stack.getObject(-1);
            found = mMetatables.emplace(type, table.store()).first;
            stack.pop(2);
        }
        return found->second;
    }

    bool UserTypeRegistry::matches(const BasicStack& stack, int index, std::type_index type) const
    {
        if (!stack.isUserData(index) || stack.isLightUserData(index))
            return false;
        auto found = mMetatables.find(type);
        if (found == mMetatables.end())
            return false;
        BasicStack& mutStack = const_cast<BasicStack&>(stack);
        mutStack.ensure(2);
        LuaApi api = stack.api();
        if (!api.pushMetatable(index))
            return false;
        found->second.pushTo(mutStack);
        bool same = api.rawEqual(-1, -2);
        api.pop(2);
        return same;
    }
}

#include "table.hpp"

#include "lua/api.hpp"

#include <format>

namespace lat
{
    int TableView::pushTableValue(int table, bool pop)
    {
        mStack.ensure(1);
        LuaApi api = mStack.api();
        api.pushTableValue(table);
        if (pop)
            api.remove(table);
        return api.getStackSize();
    }

    void TableView::setTableValue(int table)
    {
        mStack.api().setTableEntry(table);
    }

    void TableView::checkSingleValue(int prev)
    {
        const int diff = mStack.getTop() - prev;
        if (diff != 1)
        {
            if (diff > 0)
                mStack.pop(static_cast<std::uint16_t>(diff));
            throw std::runtime_error(std::format("expected a single value to be pushed got {}", diff));
        }
    }

    std::size_t TableView::size() const
    {
        return mStack.api().getObjectSize(mIndex);
    }
}

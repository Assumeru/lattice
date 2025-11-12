#include "table.hpp"

#include "lua/api.hpp"

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
}

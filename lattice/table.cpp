#include "table.hpp"

#include "lua/api.hpp"
#include "reference.hpp"

#include <format>

namespace lat
{
    int TableView::pushTableValue(int table, bool pop)
    {
        mStack.ensure(1);
        LuaApi api = mStack.api();
        if (!api.isTable(table))
            throw std::runtime_error("attempted to index non-table value");
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

    ObjectView TableView::getRaw(int index)
    {
        mStack.ensure(1);
        mStack.api().pushRawTableValue(mIndex, index);
        return mStack.getObject(-1);
    }

    void TableView::setRaw(int index, ObjectView& value)
    {
        value.pushTo(mStack);
        mStack.api().setRawTableValue(mIndex, index);
    }

    void TableView::cleanUp(int prev)
    {
        const int diff = mStack.getTop() - prev;
        if (diff > 0)
            mStack.pop(static_cast<std::uint16_t>(diff));
    }

    std::size_t TableView::size() const
    {
        return mStack.api().getObjectSize(mIndex);
    }

    TableReference TableView::store()
    {
        return TableReference(ObjectView(*this).store());
    }
}

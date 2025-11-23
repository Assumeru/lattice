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

    std::optional<std::pair<ObjectView, ObjectView>> TableView::next(ObjectView& key)
    {
        mStack.ensure(2);
        key.pushTo(mStack);
        if (!mStack.api().next(mIndex))
            return {};
        return std::make_pair(mStack.getObject(-2), mStack.getObject(-1));
    }

    TableReference TableView::store()
    {
        return TableReference(ObjectView(*this).store());
    }

    bool TableView::setEnvironment(TableView& environment)
    {
        return ObjectView(*this).setEnvironment(environment);
    }

    void TableView::setMetatable(TableView& metatable)
    {
        ObjectView(*this).setMetatable(metatable);
    }

    std::optional<ObjectView> TableView::pushMetatable()
    {
        return ObjectView(*this).pushMetatable();
    }

    TableViewIterator TableView::begin()
    {
        ObjectView key = mStack.pushNil();
        if (auto result = next(key))
        {
            Reference keyRef = result->first.store();
            Reference valueRef = result->second.store();
            mStack.pop(3);
            return TableViewIterator(*this, std::move(keyRef), std::move(valueRef));
        }
        mStack.pop();
        return end();
    }

    TableViewIterator TableView::end()
    {
        return TableViewIterator(*this);
    }

    TableViewIterator::TableViewIterator(TableView table)
        : mTable(table)
    {
    }

    TableViewIterator::TableViewIterator(TableView table, Reference key, Reference value)
        : mTable(table)
        , mKey(std::move(key))
        , mValue(std::move(value))
    {
    }

    TableViewIterator& TableViewIterator::operator++()
    {
        ObjectView key = mKey.pushTo(mTable.mStack);
        if (auto result = mTable.next(key))
        {
            mKey = result->first;
            mValue = result->second;
            mTable.mStack.pop(3);
        }
        else
        {
            mKey.reset();
            mValue.reset();
            mTable.mStack.pop();
        }
        return *this;
    }

    std::pair<const Reference&, const Reference&> TableViewIterator::operator*() const
    {
        return { mKey, mValue };
    }

    bool TableViewIterator::operator==(const TableViewIterator& other) const
    {
        return mKey == other.mKey && mValue == other.mValue;
    }
}

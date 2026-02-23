#include "table.hpp"

#include "exception.hpp"
#include "lua/api.hpp"
#include "reference.hpp"

#include <format>

namespace lat
{
    namespace
    {
        bool hasNewIndex(LuaApi& lua)
        {
            lua.pushString(meta::newIndex);
            lua.pushTableValue(-2);
            const bool hasMetaNewIndex = lua.isFunction(-1);
            lua.pop(2);
            return hasMetaNewIndex;
        }
    }

    int TableLikeViewBase::pushTableValue(int table, bool pop) const
    {
        if (!mStack.isTableLike(table))
            throw TypeError("table");
        mStack.ensure(1);
        LuaApi api = mStack.api();
        api.pushTableValue(table);
        if (pop)
            api.remove(table);
        return api.getStackSize();
    }

    void TableLikeViewBase::setTableValue(int table) const
    {
        LuaApi api = mStack.api();
        if (!api.isTable(table))
        {
            mStack.ensure(2);
            if (!api.pushMetatable(table) || !hasNewIndex(api))
                throw TypeError("table");
        }
        api.setTableEntry(table);
    }

    void TableLikeViewBase::checkSingleValue(int prev) const
    {
        const int diff = mStack.getTop() - prev;
        if (diff != 1)
        {
            if (diff > 0)
                mStack.pop(static_cast<std::uint16_t>(diff));
            throw std::runtime_error(std::format("expected a single value to be pushed got {}", diff));
        }
    }

    ObjectView TableView::getRaw(int index) const
    {
        mStack.ensure(1);
        mStack.api().pushRawTableValue(mIndex, index);
        return mStack.getObject(-1);
    }

    void TableView::setRaw(int index, const ObjectView& value) const
    {
        value.pushTo(mStack);
        mStack.api().setRawTableValue(mIndex, index);
    }

    void TableLikeViewBase::cleanUp(int prev) const
    {
        const int diff = mStack.getTop() - prev;
        if (diff > 0)
            mStack.pop(static_cast<std::uint16_t>(diff));
    }

    std::ptrdiff_t TableLikeView::size() const
    {
        LuaApi api = mStack.api();
        if (api.isTable(mIndex))
            return api.getObjectSize(mIndex);
        mStack.ensure(2);
        if (!api.pushMetatable(mIndex))
            throw TypeError("table");
        api.pushString(meta::len);
        api.pushTableValue(-2);
        mStack.remove(-2);
        api.pushCopy(mIndex);
        mStack.protectedCall(1, 1);
        const std::ptrdiff_t size = mStack.getObject(-1).asInt();
        mStack.pop();
        return size;
    }

    std::size_t TableView::size() const
    {
        return mStack.api().getObjectSize(mIndex);
    }

    std::optional<std::pair<ObjectView, ObjectView>> TableView::next(ObjectView& key) const
    {
        mStack.ensure(2);
        key.pushTo(mStack);
        if (!mStack.api().next(mIndex))
            return {};
        return std::make_pair(mStack.getObject(-2), mStack.getObject(-1));
    }

    Reference TableLikeView::store() const
    {
        return ObjectView(*this).store();
    }

    TableReference TableView::store() const
    {
        return TableReference(ObjectView(*this).store());
    }

    bool TableLikeViewBase::setEnvironment(const TableView& environment) const
    {
        return ObjectView(*this).setEnvironment(environment);
    }

    void TableLikeViewBase::setMetatable(const TableView& metatable) const
    {
        ObjectView(*this).setMetatable(metatable);
    }

    std::optional<TableView> TableLikeViewBase::pushMetatable() const
    {
        return ObjectView(*this).pushMetatable();
    }

    TableViewIterator TableView::begin() const
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

    TableViewIterator TableView::end() const
    {
        return TableViewIterator(*this);
    }

    void TableView::forEach(FunctionRef<void(ObjectView, ObjectView)> func) const
    {
        ObjectView key = mStack.pushNil();
        const int pos = mStack.getTop();
        while (auto result = next(key))
        {
            func(result->first, result->second);
            mStack.remove(pos);
            mStack.remove(pos + 1);
        }
        mStack.remove(pos);
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

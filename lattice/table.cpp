#include "table.hpp"

#include "lua/api.hpp"
#include "object.hpp"
#include "stack.hpp"

#include <cassert>
#include <span>

namespace lat
{
    namespace
    {
        void pushSegment(LuaApi& api, auto segment)
        {
            using S = decltype(segment);
            if constexpr (std::is_same_v<S, std::string_view>)
            {
                api.pushString(segment);
            }
            else if constexpr (std::is_same_v<S, std::ptrdiff_t>)
            {
                api.pushInteger(segment);
            }
            else if constexpr (std::is_same_v<S, int>)
            {
                api.pushCopy(segment);
            }
            else
            {
                static_assert(false, "non exhaustive visitor");
            }
        }

        int walk(LuaApi& api, int table, std::span<const TableIndex> path)
        {
            for (std::size_t i = 0; i < path.size(); ++i)
            {
                std::visit(
                    [&](auto segment) {
                        pushSegment(api, segment);
                        api.pushTableValue(table);
                        if (i > 0)
                            api.remove(table);
                        table = api.getStackSize();
                    },
                    path[i]);
            }
            return table;
        }
    }

    void IndexedTableView::set(ObjectView& value)
    {
        int index = value.on(mTable.mStack).mIndex;
        LuaApi api = mTable.mStack.api();
        mTable.mStack.ensure(3);
        int table = walk(api, mTable.mIndex, { mPath.begin(), mPath.end() - 1 });
        std::visit([&](auto segment) { pushSegment(api, segment); }, mPath.back());
        api.pushCopy(index);
        api.setTableEntry(table);
    }

    ObjectView IndexedTableView::get()
    {
        LuaApi api = mTable.mStack.api();
        mTable.mStack.ensure(2);
        int index = walk(api, mTable.mIndex, mPath);
        return ObjectView(mTable.mStack, index);
    }

    IndexedTableView IndexedTableView::append(IndexedTableView path, TableIndex key)
    {
        path.mPath.emplace_back(key);
        return path;
    }

    IndexedTableView IndexedTableView::operator[](std::string_view key)
    {
        return append(*this, key);
    }

    IndexedTableView IndexedTableView::operator[](std::ptrdiff_t index)
    {
        return append(*this, index);
    }

    IndexedTableView IndexedTableView::operator[](ObjectView& key)
    {
        int index = key.on(mTable.mStack).mIndex;
        return append(*this, index);
    }

    IndexedTableView TableView::operator[](std::string_view key)
    {
        return IndexedTableView::append(IndexedTableView(*this), key);
    }

    IndexedTableView TableView::operator[](std::ptrdiff_t index)
    {
        return IndexedTableView::append(IndexedTableView(*this), index);
    }

    IndexedTableView TableView::operator[](ObjectView& key)
    {
        int index = key.on(mStack).mIndex;
        return IndexedTableView::append(IndexedTableView(*this), index);
    }
}

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

        int walk(Stack& stack, LuaApi& api, int table, std::span<const TableIndex> path)
        {
            stack.ensure(3);
            for (std::size_t i = 0; i < path.size(); ++i)
            {
                std::visit(
                    [&](auto segment) {
                        pushSegment(api, segment);
                        api.pushTableValue(table);
                        if (i > 0)
                            api.remove(table);
                        table = stack.makeAbsolute(-1);
                    },
                    path[i]);
            }
            return table;
        }
    }

    void IndexedTableView::set(const ObjectView& value)
    {
        assert(&mTable.mStack == &value.mStack);
        LuaApi api = mTable.mStack.api();
        int table = walk(mTable.mStack, api, mTable.mIndex, { mPath.begin(), mPath.end() - 1 });
        std::visit([&](auto segment) { pushSegment(api, segment); }, mPath.back());
        api.pushCopy(value.mIndex);
        api.setTableEntry(table);
    }

    ObjectView IndexedTableView::get()
    {
        LuaApi api = mTable.mStack.api();
        int index = walk(mTable.mStack, api, mTable.mIndex, mPath);
        return ObjectView(mTable.mStack, index);
    }

    void IndexedTableView::operator=(const ObjectView& value)
    {
        return set(value);
    }

    IndexedTableView::operator ObjectView()
    {
        return get();
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

    IndexedTableView IndexedTableView::operator[](const ObjectView& key)
    {
        assert(&mTable.mStack == &key.mStack);
        return append(*this, key.mIndex);
    }

    IndexedTableView TableView::operator[](std::string_view key)
    {
        return IndexedTableView::append(IndexedTableView(*this), key);
    }

    IndexedTableView TableView::operator[](std::ptrdiff_t index)
    {
        return IndexedTableView::append(IndexedTableView(*this), index);
    }

    IndexedTableView TableView::operator[](const ObjectView& key)
    {
        assert(&mStack == &key.mStack);
        return IndexedTableView::append(IndexedTableView(*this), key.mIndex);
    }
}

#ifndef LATTICE_TABLE_H
#define LATTICE_TABLE_H

#include "convert.hpp"
#include "object.hpp"

#include <cstddef>
#include <utility>

namespace lat
{
    template <class Path>
    class IndexedTableView;

    class TableView
    {
        BasicStack& mStack;
        int mIndex;

        int pushTableValue(int table, bool pop);
        void setTableValue(int table);

        template <class... Path>
        ObjectView getImpl(Path&&... path)
        {
            std::size_t i = 0;
            int table = mIndex;
            (
                [&] {
                    pushToStack(mStack, std::forward<Path>(path));
                    table = pushTableValue(table, i > 0);
                    ++i;
                }(),
                ...);
            return ObjectView(mStack, table);
        }

        template <class Value, class... Path>
        void setImpl(Value&& value, Path&&... path)
        {
            constexpr std::size_t count = sizeof...(path);
            std::size_t i = 0;
            int table = mIndex;
            (
                [&] {
                    pushToStack(mStack, std::forward<Path>(path));
                    if (i == count - 1)
                    {
                        pushToStack(mStack, std::forward<Value>(value));
                        setTableValue(table);
                    }
                    else
                    {
                        table = pushTableValue(table, i > 0);
                    }
                    ++i;
                }(),
                ...);
        }

    public:
        TableView(BasicStack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        operator ObjectView() noexcept { return ObjectView(mStack, mIndex); }

        template <class Key, class... Path>
        ObjectView get(Key&& key, Path&&... args)
        {
            return getImpl(std::forward<Key>(key), std::forward<Path>(args)...);
        }

        template <class Value, class Key, class... Path>
        void set(Value&& value, Key&& key, Path&&... args)
        {
            setImpl(std::forward<Value>(value), std::forward<Key>(key), std::forward<Path>(args)...);
        }

        template <class Key>
        auto operator[](Key&& key);
    };

    template <class Path>
    class IndexedTableView
    {
        Path mPath;
        TableView mTable;

        IndexedTableView(const TableView& table, Path&& path)
            : mTable(table)
            , mPath(std::forward<Path>(path))
        {
        }

        friend class TableView;
        template <class>
        friend class IndexedTableView;
        template <class T, class, class>
        friend void pushValue(BasicStack&, T&&);

    public:
        template <class Key>
        auto operator[](Key&& key)
        {
            auto path = std::tuple_cat(mPath, std::make_tuple<Key>(std::forward<Key>(key)));
            return IndexedTableView<decltype(path)>(mTable, std::move(path));
        }

        ObjectView get()
        {
            return std::apply([&](auto&&... path) { return mTable.get(std::forward<decltype(path)>(path)...); }, mPath);
        }

        operator ObjectView() { return get(); }

        template <class Value>
        void set(Value&& value)
        {
            std::apply(
                [&](auto&&... path) { mTable.set(std::forward<Value>(value), std::forward<decltype(path)>(path)...); },
                mPath);
        }

        template <class Value>
        void operator=(Value&& value)
        {
            set(std::forward<Value>(value));
        }
    };

    template <class Key>
    auto TableView::operator[](Key&& key)
    {
        return IndexedTableView(*this, std::make_tuple<Key>(std::forward<Key>(key)));
    }

    template <class T, class Path,
        typename = std::enable_if_t<std::is_same_v<std::remove_reference_t<T>, IndexedTableView<Path>>>>
    void pushValue(BasicStack& stack, T&& value)
    {
        value.get().pushTo(stack);
        value.mTable.mStack.pop();
    }
}

#endif

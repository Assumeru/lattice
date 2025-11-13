#ifndef LATTICE_TABLE_H
#define LATTICE_TABLE_H

#include "convert.hpp"
#include "object.hpp"

#include <cstddef>
#include <stdexcept>
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
        void checkSingleValue(int prev);
        void cleanUp(int prev);

        template <class T>
        void pushSingleObject(T&& object)
        {
            const int prev = mStack.getTop();
            pushToStack(mStack, std::forward<T>(object));
            checkSingleValue(prev);
        }

        template <class... Path>
        ObjectView getImpl(Path&&... path)
        {
            const int top = mStack.getTop();
            try
            {
                std::size_t i = 0;
                int table = mIndex;
                (
                    [&] {
                        pushSingleObject(std::forward<Path>(path));
                        table = pushTableValue(table, i > 0);
                        ++i;
                    }(),
                    ...);
                return ObjectView(mStack, table);
            }
            catch (...)
            {
                cleanUp(top);
                throw;
            }
        }

        template <class Value, class... Path>
        void setImpl(Value&& value, Path&&... path)
        {
            constexpr std::size_t count = sizeof...(path);
            const int top = mStack.getTop();
            try
            {
                std::size_t i = 0;
                int table = mIndex;
                (
                    [&] {
                        pushSingleObject(std::forward<Path>(path));
                        if (i == count - 1)
                        {
                            pushSingleObject(std::forward<Value>(value));
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
            catch (...)
            {
                cleanUp(top);
                throw;
            }
        }

        template <class>
        friend class IndexedTableView;

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

        std::size_t size() const;
    };

    namespace detail
    {
        template <class T, class U = std::remove_reference_t<T>>
        concept IndexedTable = std::is_same_v<U, IndexedTableView<typename U::type>>;
    }

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

        void pop() { mTable.mStack.pop(); }

        friend class TableView;
        template <class>
        friend class IndexedTableView;
        template <detail::IndexedTable T>
        friend void pushValue(BasicStack&, T&&);

    public:
        using type = Path;

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

    template <detail::IndexedTable T>
    void pushValue(BasicStack& stack, T&& value)
    {
        value.get().pushTo(stack);
        value.pop();
    }
}

#endif

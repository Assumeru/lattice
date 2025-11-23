#ifndef LATTICE_TABLE_H
#define LATTICE_TABLE_H

#include "basicstack.hpp"
#include "convert.hpp"
#include "object.hpp"
#include "reference.hpp"

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace lat
{
    namespace meta
    {
        constexpr inline std::string_view add = "__add";
        constexpr inline std::string_view sub = "__sub";
        constexpr inline std::string_view mul = "__mul";
        constexpr inline std::string_view div = "__div";
        constexpr inline std::string_view mod = "__mod";
        constexpr inline std::string_view pow = "__pow";
        // unary minus
        constexpr inline std::string_view unm = "__unm";
        constexpr inline std::string_view concat = "__concat";
        constexpr inline std::string_view len = "__len";
        constexpr inline std::string_view eq = "__eq";
        constexpr inline std::string_view lt = "__lt";
        constexpr inline std::string_view le = "__le";
        constexpr inline std::string_view index = "__index";
        constexpr inline std::string_view newIndex = "__newindex";
        constexpr inline std::string_view call = "__call";
        // finalizer
        constexpr inline std::string_view gc = "__gc";
        // weak references; weak keys if the value contains k, weak values if the value contains v
        constexpr inline std::string_view mode = "__mode";
        // getmetatable blocker
        constexpr inline std::string_view metatable = "__metatable";
        constexpr inline std::string_view toString = "__tostring";
    }

    template <class Path>
    class IndexedTableView;
    class TableViewIterator;

    class TableView
    {
        BasicStack& mStack;
        int mIndex;

        TableView(BasicStack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        int pushTableValue(int table, bool pop);
        void setTableValue(int table);
        void checkSingleValue(int prev);
        void cleanUp(int prev);

        template <class T>
        void pushSingleObject(T&& object)
        {
            const int prev = mStack.getTop();
            detail::pushToStack(mStack, std::forward<T>(object));
            checkSingleValue(prev);
        }

        template <detail::SingleStackPull Value, class... Path>
        Value getImpl(Path&&... path)
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
                return detail::pullFromStack<Value>(mStack, table);
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
        friend class BasicStack;
        friend class ObjectView;
        friend class TableViewIterator;

    public:
        operator ObjectView() noexcept { return ObjectView(mStack, mIndex); }

        TableReference store();

        template <detail::SingleStackPull Value = ObjectView, class Key, class... Path>
        Value get(Key&& key, Path&&... args)
        {
            return getImpl<Value>(std::forward<Key>(key), std::forward<Path>(args)...);
        }

        template <class Value, class Key, class... Path>
        void set(Value&& value, Key&& key, Path&&... args)
        {
            setImpl(std::forward<Value>(value), std::forward<Key>(key), std::forward<Path>(args)...);
        }

        template <class Key>
        auto operator[](Key&& key);

        ObjectView getRaw(int);
        void setRaw(int, ObjectView&);

        std::size_t size() const;

        std::optional<std::pair<ObjectView, ObjectView>> next(ObjectView&);
        TableViewIterator begin();
        TableViewIterator end();
    };

    namespace detail
    {
        template <class T, class U = std::remove_reference_t<T>>
        concept IndexedTable = std::is_same_v<U, IndexedTableView<typename U::type>>;
    }

    template <class Path>
    class IndexedTableView
    {
        TableView mTable;
        Path mPath;

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
        friend void pushPreConvValue(BasicStack&, T&&);

    public:
        using type = Path;

        template <class Key>
        auto operator[](Key&& key)
        {
            auto path = std::tuple_cat(mPath, std::make_tuple<Key>(std::forward<Key>(key)));
            return IndexedTableView<decltype(path)>(mTable, std::move(path));
        }

        template <detail::SingleStackPull T = ObjectView>
        auto get()
        {
            return std::apply(
                [&](auto&&... path) { return mTable.get<T>(std::forward<decltype(path)>(path)...); }, mPath);
        }

        template <detail::SingleStackPull T>
        operator T()
        {
            return get<T>();
        }

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
    inline void pushPreConvValue(BasicStack& stack, T&& value)
    {
        value.get().pushTo(stack);
        value.pop();
    }

    inline TableView pullValue(BasicStack& stack, int& pos, Type<TableView>)
    {
        return stack.getObject(pos++).asTable();
    }

    inline bool isValue(const BasicStack& stack, int& pos, Type<TableView>)
    {
        return stack.isTable(pos++);
    }

    inline TableView getValue(ObjectView view, Type<TableView>)
    {
        return view.asTable();
    }

    namespace detail
    {
        template <>
        constexpr inline bool pullsOneValue<TableView> = true;
    }

    class TableViewIterator
    {
        TableView mTable;
        Reference mKey;
        Reference mValue;

        TableViewIterator(TableView);
        TableViewIterator(TableView, Reference, Reference);

        friend class TableView;

    public:
        TableViewIterator& operator++();

        std::pair<const Reference&, const Reference&> operator*() const;

        bool operator==(const TableViewIterator&) const;
    };
}

#endif

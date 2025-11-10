#ifndef LATTICE_TABLE_H
#define LATTICE_TABLE_H

#include "object.hpp"

#include <cstddef>
#include <string_view>
#include <variant>
#include <vector>

namespace lat
{
    class IndexedTableView;
    class Stack;

    using TableIndex = std::variant<std::string_view, std::ptrdiff_t, int>;

    class TableView
    {
        Stack& mStack;
        int mIndex;

        friend class IndexedTableView;

    public:
        TableView(Stack& stack, int index)
            : mStack(stack)
            , mIndex(index)
        {
        }

        IndexedTableView operator[](std::string_view key);
        IndexedTableView operator[](std::ptrdiff_t index);
        IndexedTableView operator[](ObjectView& key);
    };

    class IndexedTableView
    {
        std::vector<TableIndex> mPath;
        TableView mTable;

        IndexedTableView(const TableView& table)
            : mTable(table)
        {
        }

        friend class TableView;

        static IndexedTableView append(IndexedTableView, TableIndex);

    public:
        IndexedTableView operator[](std::string_view key);
        IndexedTableView operator[](std::ptrdiff_t index);
        IndexedTableView operator[](ObjectView& key);

        ObjectView get();
        operator ObjectView() { return get(); }

        void set(ObjectView& value);
        void operator=(ObjectView& value) { set(value); }
    };
}

#endif

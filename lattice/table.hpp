#ifndef LATTICE_TABLE_H
#define LATTICE_TABLE_H

#include <cstddef>
#include <string_view>
#include <variant>
#include <vector>

namespace lat
{
    class Stack;
    class ObjectView;
    class TableView;

    using TableIndex = std::variant<std::string_view, std::ptrdiff_t, int>;

    class IndexedTableView
    {
        TableView& mTable;
        std::vector<TableIndex> mPath;

        IndexedTableView(TableView& table)
            : mTable(table)
        {
        }

        friend class TableView;

        static IndexedTableView append(IndexedTableView, TableIndex);

    public:
        IndexedTableView operator[](std::string_view key);
        IndexedTableView operator[](std::ptrdiff_t index);
        IndexedTableView operator[](const ObjectView& key);

        ObjectView get();
        operator ObjectView();

        void set(const ObjectView& value);
        void operator=(const ObjectView& value);
    };

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
        IndexedTableView operator[](const ObjectView& key);
    };
}

#endif

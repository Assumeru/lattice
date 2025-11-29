#include "reference.hpp"

#include "lua/api.hpp"
#include "stack.hpp"

#include <utility>

namespace lat
{
    Reference::Reference()
        : mState(nullptr)
        , mRef(LUA_NOREF)
    {
    }

    Reference::Reference(Reference&& other)
    {
        mState = other.mState;
        mRef = other.mRef;
        other.mRef = LUA_NOREF;
    }

    Reference::Reference(FunctionReference&& other)
        : Reference(std::move(other.mReference))
    {
    }

    Reference::Reference(TableReference&& other)
        : Reference(std::move(other.mReference))
    {
    }

    Reference::~Reference()
    {
        reset();
    }

    void Reference::reset()
    {
        if (mRef == LUA_NOREF)
            return;
        else if (mRef != LUA_REFNIL)
            LuaApi(*mState).removeReferenceFrom(LUA_REGISTRYINDEX, mRef);
        mRef = LUA_NOREF;
    }

    bool Reference::isValid() const
    {
        return mRef != LUA_NOREF;
    }

    ObjectView Reference::pushTo(Stack& stack) const
    {
        if (mRef == LUA_NOREF)
            throw std::runtime_error("invalid reference");
        else if (mRef == LUA_REFNIL)
            return stack.pushNil();
        TableView registry = stack.getObject(LUA_REGISTRYINDEX).asTable();
        return registry.getRaw(mRef);
    }

    void Reference::onStack(FunctionRef<void(Stack&, ObjectView)> function)
    {
        if (mRef == LUA_NOREF || !mState)
            throw std::runtime_error("invalid reference");
        Stack(mState).call([&](Stack& stack) {
            ObjectView view = pushTo(stack);
            function(stack, view);
        });
    }

    void Reference::operator=(ObjectView& object)
    {
        if (!mState)
        {
            *this = object.store();
        }
        else if (object.isNil())
        {
            reset();
            mRef = LUA_REFNIL;
        }
        else if (mRef == LUA_NOREF || mRef == LUA_REFNIL)
        {
            *this = object.store();
        }
        else
        {
            TableView registry = object.mStack.getObject(LUA_REGISTRYINDEX).asTable();
            registry.setRaw(mRef, object);
        }
    }

    Reference& Reference::operator=(Reference&& other)
    {
        reset();
        swap(*this, other);
        return *this;
    }

    Reference& Reference::operator=(FunctionReference&& other)
    {
        return *this = std::move(other.mReference);
    }

    Reference& Reference::operator=(TableReference&& other)
    {
        return *this = std::move(other.mReference);
    }

    ObjectView Reference::operator=(ObjectView&& object)
    {
        ObjectView view = object;
        *this = view;
        return view;
    }

    FunctionReference::FunctionReference(Reference&& ref)
        : mReference(std::move(ref))
    {
    }

    void FunctionReference::reset()
    {
        mReference.reset();
    }

    bool FunctionReference::isValid() const
    {
        return mReference.isValid();
    }

    FunctionView FunctionReference::pushTo(Stack& stack) const
    {
        return mReference.pushTo(stack).asFunction();
    }

    void FunctionReference::onStack(FunctionRef<void(Stack&, FunctionView)> function)
    {
        mReference.onStack([&](Stack& stack, ObjectView view) { function(stack, view.asFunction()); });
    }

    void FunctionReference::operator=(FunctionView& object)
    {
        mReference = ObjectView(object);
    }

    FunctionView FunctionReference::operator=(FunctionView&& object)
    {
        FunctionView view = object;
        *this = view;
        return view;
    }

    TableReference::TableReference(Reference&& ref)
        : mReference(std::move(ref))
    {
    }

    void TableReference::reset()
    {
        mReference.reset();
    }

    bool TableReference::isValid() const
    {
        return mReference.isValid();
    }

    TableView TableReference::pushTo(Stack& stack) const
    {
        return mReference.pushTo(stack).asTable();
    }

    void TableReference::onStack(FunctionRef<void(Stack&, TableView)> function)
    {
        mReference.onStack([&](Stack& stack, ObjectView view) { function(stack, view.asTable()); });
    }

    void TableReference::operator=(TableView& object)
    {
        mReference = ObjectView(object);
    }

    TableView TableReference::operator=(TableView&& object)
    {
        TableView view = object;
        *this = view;
        return view;
    }

    void swap(Reference& l, Reference& r)
    {
        std::swap(l.mState, r.mState);
        std::swap(l.mRef, r.mRef);
    }

    bool operator==(const Reference& l, const Reference& r)
    {
        if (!l.isValid())
            return !r.isValid();
        if (l.mRef != r.mRef)
            return false;
        return l.mRef == LUA_REFNIL || l.mState == r.mState;
    }

    bool operator==(const Reference& l, const FunctionReference& r)
    {
        return l == r.mReference;
    }

    bool operator==(const FunctionReference& l, const FunctionReference& r)
    {
        return l.mReference == r.mReference;
    }

    bool operator==(const TableReference& l, const FunctionReference& r)
    {
        return l.mReference == r.mReference;
    }

    bool operator==(const Reference& l, const TableReference& r)
    {
        return l == r.mReference;
    }

    bool operator==(const TableReference& l, const TableReference& r)
    {
        return l.mReference == r.mReference;
    }
}

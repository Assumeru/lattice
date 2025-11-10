#include <stack.hpp>
#include <state.hpp>

#include <lua.hpp>

#include <cstdlib>

#include <gtest/gtest.h>

namespace
{
    using namespace lat;

    struct AllocatorData
    {
        std::size_t mAllocated = 0;
        bool mBlock = false;
    };

    void* allocate(AllocatorData* state, void* ptr, std::size_t oSize, std::size_t nSize)
    {
        if (state->mBlock && nSize > oSize)
            return nullptr;
        if (nSize == 0)
        {
            state->mAllocated -= oSize;
            std::free(ptr);
            return nullptr;
        }
        ptr = std::realloc(ptr, nSize);
        if (ptr)
        {
            state->mAllocated -= oSize;
            state->mAllocated += nSize;
        }
        return ptr;
    }

    struct MemoryTest : public testing::Test
    {
        AllocatorData mData;
        State mState;

        MemoryTest()
            : mState(allocate, &mData)
        {
        }
    };

    TEST_F(MemoryTest, can_grow_stack)
    {
        EXPECT_NO_THROW(mState.withStack([&](Stack& stack) {
            std::size_t allocated = mData.mAllocated;
            stack.ensure(LUAI_MAXCSTACK);
            EXPECT_GT(mData.mAllocated, allocated);
        }));
    }

    TEST_F(MemoryTest, exceeding_max_stack_throws)
    {
        EXPECT_ANY_THROW(mState.withStack([&](Stack& stack) { stack.ensure(LUAI_MAXCSTACK + 1); }));
    }
}

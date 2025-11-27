#include "exception.hpp"

#include <format>

namespace lat
{
    TypeError::TypeError(std::string_view type)
        : std::runtime_error(std::format("value is not of type {}", type))
        , mType(type)
    {
    }

    ArgumentTypeError::ArgumentTypeError(const char* type, int index)
        : std::runtime_error(std::format("argument #{} is not of type {}", index, type))
        , mType(type)
        , mIndex(index)
    {
    }
}

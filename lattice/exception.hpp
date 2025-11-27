#ifndef LATTICE_EXCEPTION_H
#define LATTICE_EXCEPTION_H

#include <stdexcept>
#include <string_view>

namespace lat
{
    class TypeError : public std::runtime_error
    {
        std::string_view mType;

    public:
        explicit TypeError(std::string_view expectedType);

        TypeError(const TypeError&) noexcept = default;

        std::string_view getType() const noexcept { return mType; }
    };

    class ArgumentTypeError : public std::runtime_error
    {
        std::string_view mType;
        int mIndex;

    public:
        ArgumentTypeError(const char* expectedType, int index);

        ArgumentTypeError(const ArgumentTypeError&) noexcept = default;

        std::string_view getType() const noexcept { return mType; }

        int getIndex() const noexcept { return mIndex; }
    };
}

#endif

#ifndef LATTICE_FUNCTIONREF_H
#define LATTICE_FUNCTIONREF_H

#if __cpp_lib_function_ref >= 202306L
#include <functional>

namespace lat
{
    template <class T>
    using FunctionRef = std::function_ref<T>;
}
#else

#include "extern/std23/function_ref.h"

namespace lat
{
    template <class T>
    using FunctionRef = std23::function_ref<T>;
}

#endif

#endif

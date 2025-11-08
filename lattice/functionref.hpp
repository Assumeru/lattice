#ifndef LATTICE_FUNCTIONREF_H
#define LATTICE_FUNCTIONREF_H

#if __cpp_lib_function_ref >= 202306L
#include <functional>

namespace lat
{
    template <class R, class... Args>
    using FunctionRef = std::function_ref<R(Args...)>;
}
#else

#include <memory>
#include <type_traits>

namespace lat
{
    template <class...>
    class FunctionRef;
    template <class R, class... Args>
    class FunctionRef<R(Args...)>
    {
        using FunctionPointer = R (*)(void*, Args&&...);
        void* mBound;
        FunctionPointer mFunction;

    public:
        template <class F, class T = std::remove_reference_t<F>,
            typename = std::enable_if_t<!std::is_same_v<std::remove_cv_t<F>, FunctionRef>>>
        FunctionRef(F&& function)
        {
            mBound = std::addressof(function);
            mFunction = [](void* bound, Args&&... args) -> R {
                if constexpr (std::is_same_v<R, void>)
                    (*static_cast<T*>(bound))(std::forward<Args>(args)...);
                else
                    return (*static_cast<T*>(bound))(std::forward<Args>(args)...);
            };
        }
        FunctionRef(const FunctionRef& other) = default;

        R operator()(Args&&... args) const { return mFunction(mBound, args...); }
    };
}

#endif

#endif

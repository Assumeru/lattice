add_library(LibLattice)

target_link_libraries(LibLattice PUBLIC Lua::Lua)

target_sources(LibLattice
    PRIVATE
        exception.cpp
        function.cpp
        functionref.hpp
        object.cpp
        reference.cpp
        stack.cpp
        state.cpp
        table.cpp
        userdata.cpp
        usertype.cpp
        lua/api.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            convert.hpp
            exception.hpp
            forwardstack.hpp
            function.hpp
            object.hpp
            overload.hpp
            reference.hpp
            stack.hpp
            state.hpp
            table.hpp
            userdata.hpp
            usertype.hpp
            lua/api.hpp
            lua/enums.hpp
)

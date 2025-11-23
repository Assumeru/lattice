add_library(LibLattice)

target_link_libraries(LibLattice PUBLIC Lua::Lua)

target_sources(LibLattice
    PRIVATE
        function.cpp
        functionref.hpp
        object.cpp
        reference.cpp
        stack.cpp
        state.cpp
        table.cpp
        userdata.cpp
        lua/api.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            basicstack.hpp
            convert.hpp
            function.hpp
            object.hpp
            reference.hpp
            stack.hpp
            state.hpp
            table.hpp
            userdata.hpp
            lua/api.hpp
            lua/enums.hpp
)

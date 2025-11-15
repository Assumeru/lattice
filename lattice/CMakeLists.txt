add_library(LibLattice)

target_link_libraries(LibLattice PUBLIC Lua::Lua)

target_sources(LibLattice
    PRIVATE
        function.cpp
        functionref.hpp
        object.cpp
        stack.cpp
        state.cpp
        table.cpp
        lua/api.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            basicstack.hpp
            convert.hpp
            function.hpp
            object.hpp
            stack.hpp
            state.hpp
            table.hpp
            lua/api.hpp
            lua/enums.hpp
)

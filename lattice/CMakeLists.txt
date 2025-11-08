add_library(LibLattice)

target_link_libraries(LibLattice PUBLIC Lua::Lua)

target_sources(LibLattice
    PRIVATE
        functionref.hpp
        stack.cpp
        state.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            stack.hpp
            state.hpp
            lua/api.hpp
            lua/enums.hpp
)

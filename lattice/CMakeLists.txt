add_library(LibLattice)

target_link_libraries(LibLattice PUBLIC Lua::Lua)

target_sources(LibLattice
    PRIVATE
        state.cpp
    PUBLIC
        FILE_SET HEADERS
        FILES
            state.hpp
            lua/api.hpp
            lua/enums.hpp
)

add_executable(LatticeTests)

target_link_libraries(LatticeTests LibLattice)

target_sources(LatticeTests
    PRIVATE
        main.cpp
)

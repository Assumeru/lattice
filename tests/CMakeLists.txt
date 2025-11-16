add_executable(LatticeTests)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        52eb8108c5bdec04579160ae17225d66034bd723 # release-1.17.0
)
if (MSVC)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()
FetchContent_MakeAvailable(googletest)

target_link_libraries(LatticeTests LibLattice GTest::gtest)

target_sources(LatticeTests
    PRIVATE
        conversion.cpp
        debug.cpp
        function.cpp
        library.cpp
        main.cpp
        memory.cpp
        table.cpp
)

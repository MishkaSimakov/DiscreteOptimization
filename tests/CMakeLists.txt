include(FetchContent)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/52eb8108c5bdec04579160ae17225d66034bd723.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_target(tsp_tests
        tsp/TourStorageTests.cpp
)

target_link_libraries(tsp_tests GTest::gtest_main)
set_target_properties(tsp_tests PROPERTIES CMAKE_CXX_FLAGS "-fsanitize=address")

include(GoogleTest)
gtest_discover_tests(tsp_tests)
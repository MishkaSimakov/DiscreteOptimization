cmake_minimum_required(VERSION 3.31)
project(DiscreteOptimization)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib/raspisator)

add_executable(DiscreteOptimization src/main.cpp)
target_include_directories(DiscreteOptimization PUBLIC src)
target_link_libraries(DiscreteOptimization raspisator)

target_compile_definitions(
        DiscreteOptimization
        PUBLIC PATH_TO_TASKS="${CMAKE_CURRENT_LIST_DIR}/tasks"
)

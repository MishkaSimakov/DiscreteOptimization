cmake_minimum_required(VERSION 3.31)
project(DiscreteOptimization)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# install raspisator
FetchContent_Declare(
        raspisator
        GIT_REPOSITORY https://github.com/MishkaSimakov/raspisator.git
        GIT_TAG 674a8fde8317cc82458ecdf514caebe9adc05ca0
)

FetchContent_MakeAvailable(raspisator)

# install json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

add_compile_definitions(LOG_FILEPATH="${CMAKE_CURRENT_LIST_DIR}/log")
add_compile_definitions(RESOURCES_FILEPATH="${CMAKE_CURRENT_LIST_DIR}/resources")

add_subdirectory(src)
add_subdirectory(tests)
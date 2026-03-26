cmake_minimum_required(VERSION 3.31)
project(DiscreteOptimization)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# install raspisator
FetchContent_Declare(
        raspisator
        GIT_REPOSITORY https://github.com/MishkaSimakov/raspisator.git
        GIT_TAG c13a62b58c30f302424e4aed54735eb42cfebeb7
)

FetchContent_MakeAvailable(raspisator)

# install json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz)
FetchContent_MakeAvailable(json)

add_compile_definitions(LOG_FILEPATH="${CMAKE_CURRENT_LIST_DIR}/log")
add_compile_definitions(RESOURCES_FILEPATH="${CMAKE_CURRENT_LIST_DIR}/resources")

add_subdirectory(src)
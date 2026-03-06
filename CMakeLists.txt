cmake_minimum_required(VERSION 3.31)
project(DiscreteOptimization)

set(CMAKE_CXX_STANDARD 23)

add_subdirectory(lib/raspisator)

add_compile_definitions(LOG_FILEPATH="${CMAKE_CURRENT_LIST_DIR}/log")
add_compile_definitions(RESOURCES_FILEPATH="${CMAKE_CURRENT_LIST_DIR}/resources")

function(add_target target_name source_path)
    add_executable(${target_name} ${source_path})
    target_include_directories(${target_name} PUBLIC src)
    target_link_libraries(${target_name} raspisator)

    target_compile_definitions(
            ${target_name}
            PUBLIC PATH_TO_TASKS="${CMAKE_CURRENT_LIST_DIR}/tasks"
    )
endfunction()

function(add_task_target task_name)
    add_target(${task_name} src/${task_name}/grade.cpp)
endfunction()

add_target(main src/main.cpp)

add_task_target(setcover)
add_task_target(knapsack)
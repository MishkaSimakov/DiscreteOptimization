function(add_target target_name sources)
    add_executable(${target_name})

    target_sources(${target_name} PUBLIC ${sources})
    target_include_directories(${target_name} PUBLIC ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${target_name} raspisator nlohmann_json::nlohmann_json)

    target_compile_definitions(
            ${target_name}
            PUBLIC PATH_TO_TASKS="${CMAKE_CURRENT_LIST_DIR}/../tasks" PATH_TO_OUTPUT="${CMAKE_CURRENT_LIST_DIR}/../output"
    )
endfunction()

function(add_grade_target task_name)
    add_target(${task_name}_grade ${task_name}/grade.cpp)
endfunction()

function(add_experiments_target task_name)
    add_target(${task_name}_experiments ${task_name}/experiments.cpp)

    target_compile_options(${task_name}_experiments PRIVATE -fsanitize=address)
    target_link_options(${task_name}_experiments PRIVATE -fsanitize=address)
endfunction()

add_grade_target(setcover)
add_grade_target(knapsack)
add_grade_target(coloring)
add_grade_target(tsp)
add_grade_target(facility)
add_grade_target(vrp)

add_experiments_target(setcover)
add_experiments_target(knapsack)
add_experiments_target(coloring)
add_experiments_target(tsp)
add_experiments_target(facility)
add_experiments_target(vrp)

add_target(facility_fast_annealing facility/fast_annealing.cpp)
add_target(setcover_gridsearch setcover/grid_search.cpp)
add_target(vrp_gridsearch vrp/grid_search.cpp)
function(add_target target_name source_path)
    add_executable(${target_name} ${source_path})
    target_include_directories(${target_name} PUBLIC .)
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
endfunction()

add_grade_target(setcover)
add_grade_target(knapsack)
add_grade_target(coloring)

add_experiments_target(setcover)
add_experiments_target(knapsack)
add_experiments_target(coloring)

function(aramid_load_external_project project_def_path)
    get_filename_component(project_name ${project_def_path} NAME_WE)
    configure_file(${project_def_path} ${project_name}-download/CMakeLists.txt)
    execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-download)
    if(CHILDPROCESS_RESULT)
        message(FATAL_ERROR "CMake step for ${project_name} failed: ${result}\nstdout:\n${stdout}\n\nstderr:\n${stderr}")
    endif()
    execute_process(COMMAND ${CMAKE_COMMAND} --build .
        RESULT_VARIABLE result
        OUTPUT_VARIABLE stdout
        ERROR_VARIABLE stderr
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-download)
    if(result)
        message(FATAL_ERROR "CMake step for ${project_name} failed: ${result}\nstdout:\n${stdout}\n\nstderr:\n${stderr}")
    endif()

    add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/${project_name}-src
                    ${CMAKE_CURRENT_BINARY_DIR}/${project_name}-build
                    EXCLUDE_FROM_ALL)
endfunction()

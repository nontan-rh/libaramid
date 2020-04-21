
add_library(aramid_library_objects OBJECT
    src/condvar.c
    src/context.c
    src/deque.c
    src/executor.c
    src/hash_table.c
    src/job.c
    src/logger.c
    src/memory_allocator.c
    src/memory_region.c
    src/mutex.c
    src/parallel_for.c
    src/procedure_builder.c
    src/procedure.c
    src/promise.c
    src/random.c
    src/sequential_for.c
    src/single.c
    src/spinlock.c
    src/thread.c
    src/time.c
    )
aramid_target_setup_compile_options(aramid_library_objects)
target_include_directories(aramid_library_objects PRIVATE include)

if(BUILD_TESTING)
    add_library(aramid_unit_test_object OBJECT
        src/deque.test.cpp
        src/hash_table.test.cpp
        src/random.test.cpp
        )
    aramid_target_setup_compile_options(aramid_unit_test_object)
    target_include_directories(aramid_unit_test_object PRIVATE include $<TARGET_PROPERTY:gtest_main,INTERFACE_INCLUDE_DIRECTORIES>)
endif()

add_library(aramid $<TARGET_OBJECTS:aramid_library_objects>)
aramid_target_setup_compile_options(aramid)
target_include_directories(aramid INTERFACE include)

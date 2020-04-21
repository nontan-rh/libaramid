
# Option Defaults

if(CMAKE_C_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "GNU")
    set(DEFAULT_SPINLOCK_IMPLEMENTATION "GCCIntrinsic")
elseif(CMAKE_C_COMPILER_ID MATCHES "MSVC")
    set(DEFAULT_SPINLOCK_IMPLEMENTATION "MSVCIntrinsic")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(DEFAULT_THREAD_IMPLEMENTATION "win32")
else()
    set(DEFAULT_THREAD_IMPLEMENTATION "pthread")
endif()

# Options

option(BUILD_TESTING "Build test" ON)
option(ENABLE_ASAN "Build with ASAN support (GCC/clang and *nix required)")
option(DISABLE_MEMORY_REGION "Disable memory region feature")
set(SPINLOCK_IMPLEMENTATION ${DEFAULT_SPINLOCK_IMPLEMENTATION} CACHE STRING "Spinlock implementation (GCCIntrinsic|MSVCIntrinsic)")
set(THREAD_IMPLEMENTATION ${DEFAULT_THREAD_IMPLEMENTATION} CACHE STRING "Thread implementation (pthread|win32)")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(ARAMID_COMPILE_OPTIONS "")
set(ARAMID_COMPILE_DEFINITIONS "")

if(ENABLE_ASAN)
    # Use directly instead of target_compile_options, LINK_OPTIONS, add_link_options
    # or target_link_options because it's not available before version 3.13.0
    # list(APPEND ARAMID_COMPILE_OPTIONS -fsanitize=address)
    list(APPEND CMAKE_C_FLAGS -fsanitize=address)
    list(APPEND CMAKE_CXX_FLAGS -fsanitize=address)
endif()

if(DISABLE_MEMORY_REGION)
    list(APPEND ARAMID_COMPILE_DEFINITIONS ARAMID_DISABLE_MEMORY_REGION)
endif()

if(MSVC)
    list(APPEND ARAMID_COMPILE_OPTIONS /W4 /WX)
else()
    list(APPEND ARAMID_COMPILE_OPTIONS -Wall -Wextra -pedantic -Werror)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Windows")
    list(APPEND ARAMID_COMPILE_DEFINITIONS _WIN32_WINNT=_WIN32_WINNT_WIN8)
elseif(CMAKE_SYSTEM_NAME MATCHES "Emscripten")
    set(EMSCRIPTEN_FLAGS "-s USE_PTHREADS=1 -s PROXY_TO_PTHREAD=1 -s EXIT_RUNTIME=1 --shell-file ${CMAKE_CURRENT_SOURCE_DIR}/etc/wasm/shell.html")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_FLAGS}")
endif()

if(THREAD_IMPLEMENTATION STREQUAL "pthread")
    list(APPEND ARAMID_COMPILE_OPTIONS -pthread)
    list(APPEND ARAMID_COMPILE_DEFINITIONS ARAMID_USE_PTHREAD)
elseif(THREAD_IMPLEMENTATION STREQUAL "win32")
    list(APPEND ARAMID_COMPILE_DEFINITIONS ARAMID_USE_WIN32THREAD)
else()
    message(FATAL_ERROR "Unknown THREAD_IMPLEMENTATION: ${THREAD_IMPLEMENTATION}")
endif()

if (SPINLOCK_IMPLEMENTATION STREQUAL "GCCIntrinsic")
    list(APPEND ARAMID_COMPILE_DEFINITIONS ARAMID_USE_GCC_INTRINSIC_SPINLOCK)
elseif(SPINLOCK_IMPLEMENTATION STREQUAL "MSVCIntrinsic")
    list(APPEND ARAMID_COMPILE_DEFINITIONS ARAMID_USE_MSVC_INTRINSIC_SPINLOCK)
else()
    message(FATAL_ERROR "Unknown SPINLOCK_IMPLEMENTATION: ${SPINLOCK_IMPLEMENTATION}")
endif()

function(aramid_target_setup_compile_options target)
    set_property(TARGET ${target} PROPERTY POSITION_INDEPENDENT_CODE ON)
    target_compile_options(${target} PRIVATE ${ARAMID_COMPILE_OPTIONS})
    target_compile_definitions(${target} PRIVATE ${ARAMID_COMPILE_DEFINITIONS})
endfunction()

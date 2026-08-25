cmake_minimum_required(VERSION 3.16)
project(CacheCraft VERSION 3.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

function(enable_strict_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

add_library(cachecraft
    src/cache.cpp
    src/trace.cpp
    src/analysis.cpp
    src/reuse.cpp
    src/synthetic.cpp
)
target_include_directories(cachecraft PUBLIC include)
enable_strict_warnings(cachecraft)

add_executable(cachecraft_cli src/main.cpp)
target_link_libraries(cachecraft_cli PRIVATE cachecraft)
enable_strict_warnings(cachecraft_cli)

add_executable(cachecraft_experiments src/experiment.cpp)
target_link_libraries(cachecraft_experiments PRIVATE cachecraft)
enable_strict_warnings(cachecraft_experiments)

add_executable(cachecraft_tests tests/test_cache.cpp)
target_link_libraries(cachecraft_tests PRIVATE cachecraft)
enable_strict_warnings(cachecraft_tests)

add_executable(cachecraft_analysis_tests tests/test_analysis_v3.cpp)
target_link_libraries(cachecraft_analysis_tests PRIVATE cachecraft)
enable_strict_warnings(cachecraft_analysis_tests)

enable_testing()
add_test(NAME cachecraft_tests COMMAND cachecraft_tests)
add_test(NAME cachecraft_analysis_tests COMMAND cachecraft_analysis_tests)
add_test(NAME cachecraft_experiment_smoke COMMAND cachecraft_experiments)

install(TARGETS cachecraft cachecraft_cli cachecraft_experiments)
install(DIRECTORY include/ DESTINATION include)

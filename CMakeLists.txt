cmake_minimum_required(VERSION 3.16)
project(CacheCraft VERSION 2.0.0 LANGUAGES CXX)

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

add_library(cachecraft src/cache.cpp src/trace.cpp)
target_include_directories(cachecraft PUBLIC include)
enable_strict_warnings(cachecraft)

add_executable(cachecraft_cli src/main.cpp)
target_link_libraries(cachecraft_cli PRIVATE cachecraft)
enable_strict_warnings(cachecraft_cli)

add_executable(cachecraft_tests tests/test_cache.cpp)
target_link_libraries(cachecraft_tests PRIVATE cachecraft)
enable_strict_warnings(cachecraft_tests)

enable_testing()
add_test(NAME cachecraft_tests COMMAND cachecraft_tests)

install(TARGETS cachecraft cachecraft_cli)
install(DIRECTORY include/ DESTINATION include)

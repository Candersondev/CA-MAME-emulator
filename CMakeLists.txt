cmake_minimum_required(VERSION 3.21)
project(CA_MAME VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ca_mame_core
    src/core/Machine.cpp
    src/bus/MemoryBus.cpp
    src/cpu/Intel8080.cpp
)
target_include_directories(ca_mame_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(MSVC)
    target_compile_options(ca_mame_core PRIVATE /W4)
else()
    target_compile_options(ca_mame_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(ca-mame src/frontend/main.cpp)
target_link_libraries(ca-mame PRIVATE ca_mame_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(ca-mame-tests tests/core_tests.cpp)
    target_link_libraries(ca-mame-tests PRIVATE ca_mame_core)
    add_test(NAME core_tests COMMAND ca-mame-tests)
    add_executable(intel8080-tests tests/intel8080_tests.cpp)
    target_link_libraries(intel8080-tests PRIVATE ca_mame_core)
    add_test(NAME intel8080_tests COMMAND intel8080-tests)
endif()

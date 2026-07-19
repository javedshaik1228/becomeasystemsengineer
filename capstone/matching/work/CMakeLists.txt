cmake_minimum_required(VERSION 3.24)
project(MatchingEngineWork LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(MATCHING_STEP 1 CACHE STRING "Highest guided capstone step to test (1-10)")

if(MATCHING_STEP LESS 1 OR MATCHING_STEP GREATER 10)
  message(FATAL_ERROR "MATCHING_STEP must be between 1 and 10")
endif()

find_package(Threads REQUIRED)

add_library(matching_work
  src/order_book.cpp
  src/engine.cpp
  src/mock_data.cpp
  src/edge.cpp
)
target_include_directories(matching_work PUBLIC include)
target_link_libraries(matching_work PUBLIC Threads::Threads)
target_compile_features(matching_work PUBLIC cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(matching_work PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

add_executable(matching-work-tests tests/matching_work_tests.cpp)
target_link_libraries(matching-work-tests PRIVATE matching_work)
target_compile_definitions(matching-work-tests PRIVATE MATCHING_STEP=${MATCHING_STEP})

enable_testing()
add_test(NAME matching-step-${MATCHING_STEP} COMMAND matching-work-tests)
set_tests_properties(matching-step-${MATCHING_STEP} PROPERTIES TIMEOUT 20)

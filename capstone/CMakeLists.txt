cmake_minimum_required(VERSION 3.24)
project(NetForge VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(NETFORGE_TRACK "work" CACHE STRING "Implementation tree: work or reference")
set_property(CACHE NETFORGE_TRACK PROPERTY STRINGS work reference)
option(NETFORGE_SANITIZE_ADDRESS "Enable AddressSanitizer and UBSan" OFF)
option(NETFORGE_SANITIZE_THREAD "Enable ThreadSanitizer" OFF)

if(NOT NETFORGE_TRACK STREQUAL "work" AND NOT NETFORGE_TRACK STREQUAL "reference")
  message(FATAL_ERROR "NETFORGE_TRACK must be 'work' or 'reference'")
endif()

if(NETFORGE_SANITIZE_ADDRESS AND NETFORGE_SANITIZE_THREAD)
  message(FATAL_ERROR "ASan/UBSan and TSan must be run in separate builds")
endif()

find_package(Threads REQUIRED)
set(NETFORGE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/${NETFORGE_TRACK}")

add_library(netforge
  "${NETFORGE_ROOT}/src/client.cpp"
  "${NETFORGE_ROOT}/src/packet.cpp"
  "${NETFORGE_ROOT}/src/protocol.cpp"
  "${NETFORGE_ROOT}/src/server.cpp"
  "${NETFORGE_ROOT}/src/store.cpp"
  "${NETFORGE_ROOT}/src/thread_pool.cpp"
  "${NETFORGE_ROOT}/src/wal.cpp"
)
target_include_directories(netforge PUBLIC "${NETFORGE_ROOT}/include")
target_link_libraries(netforge PUBLIC Threads::Threads)
target_compile_definitions(netforge PRIVATE _POSIX_C_SOURCE=200809L)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(netforge PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2
  )
endif()

if(NETFORGE_SANITIZE_ADDRESS)
  target_compile_options(netforge PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(netforge PUBLIC -fsanitize=address,undefined)
endif()

if(NETFORGE_SANITIZE_THREAD)
  target_compile_options(netforge PUBLIC -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(netforge PUBLIC -fsanitize=thread)
endif()

add_executable(netforge-server "${NETFORGE_ROOT}/apps/server_main.cpp")
target_link_libraries(netforge-server PRIVATE netforge)

add_executable(netforge-client "${NETFORGE_ROOT}/apps/client_main.cpp")
target_link_libraries(netforge-client PRIVATE netforge)

add_executable(netforge-packet "${NETFORGE_ROOT}/apps/packet_main.cpp")
target_link_libraries(netforge-packet PRIVATE netforge)

add_executable(netforge-bench "${NETFORGE_ROOT}/apps/bench_main.cpp")
target_link_libraries(netforge-bench PRIVATE netforge)

enable_testing()
add_executable(netforge-tests tests/netforge_tests.cpp)
target_link_libraries(netforge-tests PRIVATE netforge)

foreach(day_padded IN ITEMS 01 02 03 04 05 06 07 08 09 10 11 12)
  add_test(NAME "day${day_padded}" COMMAND netforge-tests "day${day_padded}")
  set_tests_properties("day${day_padded}" PROPERTIES
    LABELS "day${day_padded}"
    TIMEOUT 20
  )
endforeach()
add_test(NAME day14 COMMAND netforge-tests day14)
set_tests_properties(day14 PROPERTIES LABELS "day14;integration;recovery" TIMEOUT 30)

set_tests_properties(day04 PROPERTIES LABELS "day04;concurrency;stress")
set_tests_properties(day05 PROPERTIES LABELS "day05;concurrency;stress")
set_tests_properties(day07 PROPERTIES LABELS "day07;integration;networking")
set_tests_properties(day08 PROPERTIES LABELS "day08;abuse;networking")
set_tests_properties(day09 PROPERTIES LABELS "day09;ipc;process")
set_tests_properties(day10 PROPERTIES LABELS "day10;debugging")
set_tests_properties(day11 PROPERTIES LABELS "day11;recovery;storage")
set_tests_properties(day12 PROPERTIES LABELS "day12;abuse;security")

message(STATUS "NetForge track: ${NETFORGE_TRACK}")

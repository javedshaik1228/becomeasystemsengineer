cmake_minimum_required(VERSION 3.24)
project(NetForgeMatchingReference VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MATCHING_SANITIZE_ADDRESS "Enable AddressSanitizer and UBSan" OFF)
option(MATCHING_SANITIZE_THREAD "Enable ThreadSanitizer" OFF)
option(MATCHING_WITH_RDKAFKA "Enable librdkafka Kafka publishing" ON)
if(MATCHING_SANITIZE_ADDRESS AND MATCHING_SANITIZE_THREAD)
  message(FATAL_ERROR "Run ASan/UBSan and TSan in separate builds")
endif()

find_package(Threads REQUIRED)

add_library(matching_engine
  src/order_book.cpp
  src/engine.cpp
  src/mock_data.cpp
  src/redis_edge.cpp
  src/kafka_edge.cpp
)
target_include_directories(matching_engine PUBLIC include)
target_link_libraries(matching_engine PUBLIC Threads::Threads)
if(MATCHING_WITH_RDKAFKA)
  find_path(RDKAFKA_INCLUDE_DIR librdkafka/rdkafka.h)
  find_library(RDKAFKA_LIBRARY NAMES rdkafka)
  if(RDKAFKA_INCLUDE_DIR AND RDKAFKA_LIBRARY)
    target_compile_definitions(matching_engine PRIVATE MATCHING_WITH_RDKAFKA=1)
    target_include_directories(matching_engine PRIVATE ${RDKAFKA_INCLUDE_DIR})
    target_link_libraries(matching_engine PRIVATE ${RDKAFKA_LIBRARY})
  else()
    message(STATUS "librdkafka not found; KafkaProducer uses sidecar envelope mode")
  endif()
endif()
target_compile_features(matching_engine PUBLIC cxx_std_20)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  target_compile_options(matching_engine PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wformat=2)
endif()
if(MATCHING_SANITIZE_ADDRESS)
  target_compile_options(matching_engine PUBLIC -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(matching_engine PUBLIC -fsanitize=address,undefined)
endif()
if(MATCHING_SANITIZE_THREAD)
  target_compile_options(matching_engine PUBLIC -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(matching_engine PUBLIC -fsanitize=thread)
endif()

add_executable(matching-engine apps/matching_main.cpp)
target_link_libraries(matching-engine PRIVATE matching_engine)
add_executable(matching-generate apps/mock_generate_main.cpp)
target_link_libraries(matching-generate PRIVATE matching_engine)
add_executable(matching-bench apps/bench_main.cpp)
target_link_libraries(matching-bench PRIVATE matching_engine)
add_executable(matching-tests tests/matching_tests.cpp)
target_link_libraries(matching-tests PRIVATE matching_engine)

enable_testing()
add_test(NAME matching-unit COMMAND matching-tests unit)
add_test(NAME matching-ingress COMMAND matching-tests ingress)
add_test(NAME matching-mock-replay COMMAND matching-tests mock)
set_tests_properties(matching-unit matching-ingress matching-mock-replay PROPERTIES TIMEOUT 30)

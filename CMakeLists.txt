# cmake

cmake_minimum_required(VERSION 3.12)

project(hl-server)


include(cmake/version.cmake)
include(cmake/build.cmake)
include(cmake/doxygen.cmake)

add_subdirectory(third-party)

find_package(Boost COMPONENTS program_options REQUIRED)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED)

find_library(Clang_LIBRARY NAMES clang HINTS ${LLVM_LIBRARY_DIRS})

set(PROJECT_SRC
  src/main.cpp

  src/hl/TokenizeHandler.cpp
  src/hl/RequestObject.cpp
  src/hl/ResponseObject.cpp

  src/hl/tokenizer/TokenizerFactory.cpp
  src/hl/tokenizer/CppTokenizer.cpp
  src/hl/tokenizer/PolyndromTokenizer.cpp
  )

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE
  Boost::boost
  Threads::Threads
  ${Boost_LIBRARIES}
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator
  ${Clang_LIBRARY}
  stdc++fs
  ss
  )
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  ${LLVM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  )


# generate default flags for clang tokenizer
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gen/default_cpp_flags.cmake.hpp ${CMAKE_CURRENT_BINARY_DIR}/gen/default_cpp_flags.hpp)


# generate version header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gen/version.cmake.hpp ${CMAKE_CURRENT_BINARY_DIR}/gen/version.hpp)

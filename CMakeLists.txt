# cmake

cmake_minimum_required(VERSION 3.5)

project(hl-server)


include(cmake/version.cmake)
include(cmake/build.cmake)

add_subdirectory(third-party)

find_package(LLVM ${USE_LLVM_VERSION} REQUIRED)

find_library(Clang_LIBRARY NAMES clang HINTS ${LLVM_LIBRARY_DIRS})

set(PROJECT_SRC
  src/main.cpp
  src/clang_tokenize.cpp
  )

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_11)
target_link_libraries(${PROJECT_NAME} PRIVATE
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator
  ${Clang_LIBRARY}
  stdc++fs
  )
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  ${LLVM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  third-party
  )


# generate default flags for clang tokenizer
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gen/default_cpp_flags.cmake.h ${CMAKE_CURRENT_BINARY_DIR}/gen/default_cpp_flags.h)

# generate version header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gen/version.cmake.h ${CMAKE_CURRENT_BINARY_DIR}/gen/version.h)

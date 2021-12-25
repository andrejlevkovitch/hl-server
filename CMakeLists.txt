# cmake

cmake_minimum_required(VERSION 3.5)

project(hl-server)


option(GO_TOKENIZER "go syntax highlight" OFF)


include(cmake/version.cmake)
include(cmake/build.cmake)

add_subdirectory(third-party)

find_package(Threads REQUIRED)

if(DEFINED LLVM_BASE_DIR)
  message(STATUS "LLVM_BASE_DIR: ${LLVM_BASE_DIR}")
  set(LLVM_LIBRARY_DIRS ${LLVM_BASE_DIR}/lib)
  set(LLVM_INCLUDE_DIRS ${LLVM_BASE_DIR}/include)
else()
  message(STATUS "LLVM_DIR not set, try to use LLVM config")
  find_package(LLVM ${USE_LLVM_VERSION} REQUIRED)
endif()

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
  Threads::Threads
  )
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  ${LLVM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  third-party
  )

if (GO_TOKENIZER)
  add_custom_target(go_tokenizer
      COMMAND go build -buildmode c-archive -o ${CMAKE_CURRENT_BINARY_DIR}/gotokenizer.a
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/go_tokenizer"
      )
  add_dependencies(${PROJECT_NAME} go_tokenizer)

  target_compile_definitions(${PROJECT_NAME} PRIVATE GO_TOKENIZER)
  target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/gotokenizer.a
  )
endif()


# generate version header
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gen/version.cmake.h ${CMAKE_CURRENT_BINARY_DIR}/gen/version.h)


# installation
include(GNUInstallDirs)
install(TARGETS hl-server
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )

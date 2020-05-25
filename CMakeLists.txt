# cmake

cmake_minimum_required(VERSION 3.12)

project(hl-server)

include(cmake/build.cmake)
include(cmake/doxygen.cmake)

add_subdirectory(third-party)

find_package(Boost COMPONENTS program_options stacktrace_backtrace REQUIRED)
find_package(Threads REQUIRED)
find_package(LLVM REQUIRED)
find_package(Clang REQUIRED)

set(PROJECT_SRC
  src/main.cpp

  src/ss/Context.cpp
  src/ss/Server.cpp
  src/ss/Session.cpp
  src/ss/SessionPool.cpp
  src/ss/errors.cpp

  src/hl/RequestHandler.cpp
  src/hl/HandlerFactory.cpp
  src/hl/RequestObject.cpp
  src/hl/ResponseObject.cpp

  src/hl/tokenizer/TokenizerFactory.cpp
  src/hl/tokenizer/CppTokenizer.cpp
  src/hl/tokenizer/PolyndromTokenizer.cpp
  )

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_compile_options(${PROJECT_NAME} PRIVATE -g) # needed for working backtrace
target_link_libraries(${PROJECT_NAME} PRIVATE
  Boost::boost
  Threads::Threads
  ${Boost_LIBRARIES}
  nlohmann_json::nlohmann_json
  nlohmann_json_schema_validator
  libclang
  stdc++fs
  )
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  ${CLANG_INCLUDE_DIRS}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/third-party
  )

# needed for colorize logs int console
if(COLORIZED)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DCOLORIZED)
endif()


configure_file(${CMAKE_CURRENT_SOURCE_DIR}/gen/default_cpp_flags.cmake.hpp ${CMAKE_CURRENT_BINARY_DIR}/gen/default_cpp_flags.hpp)

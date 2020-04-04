# cmake

cmake_minimum_required(VERSION 3.12)

project(hl_server)

include(cmake/build.cmake)
include(cmake/doxygen.cmake)

find_package(Boost COMPONENTS program_options stacktrace_backtrace REQUIRED)
find_package(Threads REQUIRED)

set(PROJECT_SRC
  src/main.cpp

  src/ss/Context.cpp
  src/ss/Server.cpp
  src/ss/Session.cpp
  src/ss/SessionPool.cpp

  src/hl/RequestHandler.cpp
  src/hl/HandlerFactory.cpp
  src/hl/RequestObject.cpp
  src/hl/ResponseObject.cpp
  )

add_executable(${PROJECT_NAME} ${PROJECT_SRC})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE
  Boost::boost
  Threads::Threads
  ${Boost_LIBRARIES}
  )
target_include_directories(${PROJECT_NAME} PRIVATE
  include
  )

# needed for colorize logs int console
if(COLORIZED)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DCOLORIZED)
endif()

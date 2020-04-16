# cmake for third-party modules

# init and update git submodules
if(NOT EXISTS Catch2)
  find_package(Git REQUIRED)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule init)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update)
endif()

option(JSON_BuildTests OFF)
option(JSON_Install    OFF)
add_subdirectory(json)

option(BUILD_TESTS    OFF)
option(BUILD_EXAMPLES OFF)
add_subdirectory(json-schema-validator)

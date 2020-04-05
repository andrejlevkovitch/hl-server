# cmake for third-party modules

# init and update git submodules
if(NOT EXISTS Catch2)
  find_package(Git REQUIRED)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule init)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update)
endif()

set(JSON_BuildTests OFF)
set(JSON_Install    OFF)
add_subdirectory(json)

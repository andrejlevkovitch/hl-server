# cmake for third-party modules

# init and update git submodules
find_package(Git REQUIRED)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule sync)
execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --recursive --init)


set(BUILD_SHARED_LIBS OFF)

option(JSON_BuildTests "" OFF)
option(JSON_Install    "" OFF)
add_subdirectory(json)

option(BUILD_TESTS     "" OFF)
option(BUILD_EXAMPLES  "" OFF)
add_subdirectory(json-schema-validator)

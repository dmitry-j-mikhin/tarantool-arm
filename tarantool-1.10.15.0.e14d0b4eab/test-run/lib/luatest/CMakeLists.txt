cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

project(luatest NONE)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Find Tarantool and Lua dependecies
set(Tarantool_FIND_REQUIRED ON)
find_package(Tarantool)

## VERSION ####################################################################
###############################################################################

if (NOT VERSION)
  execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_STRIP_TRAILING_WHITESPACE
    OUTPUT_VARIABLE VERSION
    ERROR_QUIET
  )
endif()

if (VERSION)
  configure_file (
    "${PROJECT_SOURCE_DIR}/${PROJECT_NAME}/VERSION.lua.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}/VERSION.lua"
  )
elseif(NOT LUAROCKS) # allow luarocks to build without passing VERSION
  message(FATAL_ERROR "VERSION is not provided")
endif()

## Dependencies and custom targets ############################################
###############################################################################

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES build)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES .rocks)
add_custom_command(
  OUTPUT .rocks
  DEPENDS ${PROJECT_NAME}-scm-1.rockspec
  COMMAND tarantoolctl rocks make ./${PROJECT_NAME}-scm-1.rockspec
  COMMAND tarantoolctl rocks install http 1.1.0
  COMMAND tarantoolctl rocks install luacheck 0.25.0
  COMMAND tarantoolctl rocks install luacov 0.13.0
)

add_custom_target(bootstrap DEPENDS .rocks)

add_custom_command(
  OUTPUT .rocks/bin/ldoc
  DEPENDS bootstrap
  COMMAND tarantoolctl rocks install ldoc --server=http://rocks.moonscript.org
)

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES doc)
add_custom_target(doc
  DEPENDS .rocks/bin/ldoc
  COMMAND .rocks/bin/ldoc -t ${PROJECT_NAME}-scm-1 -p ${PROJECT_NAME} --all .
)

add_custom_target(lint DEPENDS bootstrap COMMAND .rocks/bin/luacheck .)
add_custom_target(selftest DEPENDS bootstrap COMMAND bin/luatest)

add_custom_target(test_with_coverage_report
  DEPENDS bootstrap
  COMMAND rm -f tmp/luacov.*.out*
  COMMAND tarantool -l luatest.coverage bin/luatest
  COMMAND .rocks/bin/luacov .
  COMMAND echo
  COMMAND grep -A999 '^Summary' tmp/luacov.report.out
)

## Test #######################################################################
###############################################################################

enable_testing()
add_test(lint ${CMAKE_MAKE_PROGRAM} lint)
add_test(selftest ${CMAKE_MAKE_PROGRAM} selftest)

## Install ####################################################################
###############################################################################

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}
            ${CMAKE_CURRENT_SOURCE_DIR}/luarocks
  USE_SOURCE_PERMISSIONS
  DESTINATION ${TARANTOOL_INSTALL_LUADIR}/
  PATTERN "*.in" EXCLUDE
)

install(
  DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}
  DESTINATION ${TARANTOOL_INSTALL_LUADIR}/
  OPTIONAL
)

install(
  PROGRAMS ${CMAKE_CURRENT_SOURCE_DIR}/bin/luatest
  DESTINATION ${TARANTOOL_INSTALL_BINDIR}
)

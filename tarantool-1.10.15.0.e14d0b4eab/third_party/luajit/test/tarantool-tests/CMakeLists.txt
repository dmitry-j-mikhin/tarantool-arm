# Test suite that has been moved from Tarantool repository in
# scope of https://github.com/tarantool/tarantool/issues/4478.

# See the rationale in the root CMakeLists.txt.
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

find_program(PROVE prove)
if(NOT PROVE)
  message(WARNING "`prove' is not found, so tarantool-tests target is not generated")
  return()
endif()

macro(BuildTestCLib lib sources)
  add_library(${lib} SHARED EXCLUDE_FROM_ALL ${sources})
  target_include_directories(${lib} PRIVATE
    ${LUAJIT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  set_target_properties(${lib} PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    PREFIX ""
  )

  # XXX: This change affects the current cmake variable scope and
  # so a user should care to don't use it in a top level scope.
  # The dynamic libraries are loaded with LuaJIT binary and use
  # symbols from it. So it is totally OK to have unresolved
  # symbols at build time.
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set_target_properties(${lib} PROPERTIES
      LINK_FLAGS "-undefined dynamic_lookup"
    )
  else()
    # FIXME: Unfortunately, there is no another way to suppress
    # this linker option, so just strip it out from the flags.
    string(REPLACE "-Wl,--no-undefined" ""
      CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}"
    )
  endif()
  # XXX: Append the lib to be built to the dependency list.
  # Unfortunately, there is no convenient way in CMake to extend
  # the list in parent scope other than join two strings with
  # semicolon. If one finds the normal way to make it work, feel
  # free to reach me.
  set(TESTLIBS "${lib};${TESTLIBS}" PARENT_SCOPE)
  # Add the directory where the lib is built to the list with
  # entries for LUA_CPATH environment variable, so LuaJIT can find
  # and load it. See the comment about extending the list in the
  # parent scope few lines above.
  set(LUA_CPATHS "${CMAKE_CURRENT_BINARY_DIR}/?${CMAKE_SHARED_LIBRARY_SUFFIX};${LUA_CPATHS}" PARENT_SCOPE)
  # Also add this directory to LD_LIBRARY_PATH environment
  # variable, so FFI machinery can find and load it.
  set(LD_LIBRARY_PATH "${CMAKE_CURRENT_BINARY_DIR}:${LD_LIBRARY_PATH}" PARENT_SCOPE)
endmacro()

add_subdirectory(gh-4427-ffi-sandwich)
add_subdirectory(lj-416-xor-before-jcc)
add_subdirectory(lj-601-fix-gc-finderrfunc)
add_subdirectory(lj-flush-on-trace)

# JIT, profiler and bytecode toolchains are located in src/
# directory, jit/vmdef.lua is autogenerated file also located in
# src/ directory, but in the scope of the binary artefacts tree
# and auxiliary tests-related modules are located in the current
# directory (but tests are run in the binary directory), so
# LUA_PATH need to be updated.
make_lua_path(LUA_PATH
  PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/?.lua
    ${LUAJIT_SOURCE_DIR}/?.lua
    ${LUAJIT_BINARY_DIR}/?.lua
)
# Update LUA_CPATH with the library paths collected within
# <BuildTestLib> macro.
make_lua_path(LUA_CPATH PATHS ${LUA_CPATHS})

set(LUA_TEST_SUFFIX .test.lua)
set(LUA_TEST_FLAGS --failures --shuffle)
set(LUA_TEST_ENV
  "LUA_PATH=\"${LUA_PATH}\""
  "LUA_CPATH=\"${LUA_CPATH}\""
)

if(CMAKE_VERBOSE_MAKEFILE)
  list(APPEND LUA_TEST_FLAGS --verbose)
endif()

# XXX: Since the auxiliary libraries are built as a dynamically
# loaded modules on MacOS instead of shared libraries as it is
# done on Linux and BSD, another environment variable should be
# used to guide <ffi.load> while searching the extension.
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  # XXX: Apple tries their best to "protect their users from
  # malware". As a result SIP (see the link[1] below) has been
  # designed and released. Now, Apple developers are so protected,
  # that they can load nothing being not installed in the system,
  # since some programs sanitize the environment before they start
  # child processes. Specifically, environment variables starting
  # with DYLD_ and LD_ are unset for child process started by
  # other programs (like /usr/bin/prove --exec using for launching
  # test suite). For more info, see the docs[2] below.
  #
  # These environment variables are used by FFI machinery to find
  # the proper shared library, hence we can still tweak testing
  # environment before calling <ffi.load>. However, the value
  # can't be passed via the standard environment variable, so we
  # use env call in prove's --exec flag value to get around SIP
  # magic tricks.
  #
  # [1]: https://support.apple.com/en-us/HT204899
  # [2]: https://developer.apple.com/library/archive/documentation/Security/Conceptual/System_Integrity_Protection_Guide/RuntimeProtections/RuntimeProtections.html
  list(APPEND LUA_TEST_ENV_MORE DYLD_LIBRARY_PATH="${LD_LIBRARY_PATH}")
else()
  list(APPEND LUA_TEST_ENV_MORE LD_LIBRARY_PATH="${LD_LIBRARY_PATH}")
endif()

# LUA_CPATH and LD_LIBRARY_PATH variables and also TESTLIBS list
# with dependecies are set in scope of BuildTestLib macro.
add_custom_target(tarantool-tests
  DEPENDS ${LUAJIT_TEST_BINARY} ${TESTLIBS}
)
add_custom_command(TARGET tarantool-tests
  COMMENT "Running Tarantool tests"
  COMMAND
  # XXX: We can't move everything to the "inner" env, since there
  # are some issues with escaping ';' for different shells. As
  # a result LUA_PATH/LUA_CPATH variables are set via the "outer"
  # env, since they are not stripped by SIP like LD_*/DYLD_* are.
  env
    ${LUA_TEST_ENV}
    ${PROVE} ${CMAKE_CURRENT_SOURCE_DIR}
      --exec 'env ${LUA_TEST_ENV_MORE} ${LUAJIT_TEST_COMMAND}'
      --ext ${LUA_TEST_SUFFIX}
      --jobs ${CMAKE_BUILD_PARALLEL_LEVEL}
      ${LUA_TEST_FLAGS}
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)


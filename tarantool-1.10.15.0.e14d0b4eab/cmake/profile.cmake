set(CMAKE_REQUIRED_FLAGS "-fprofile-arcs -ftest-coverage")
if (CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_VERSION VERSION_LESS 11)
    check_library_exists("" __gcov_dump "" HAVE_GCOV)
    if (HAVE_GCOV)
        check_library_exists("" __gcov_reset "" HAVE_GCOV)
      endif()
else()
    check_library_exists("" __gcov_flush "" HAVE_GCOV)
endif()
set(CMAKE_REQUIRED_FLAGS "")

set(ENABLE_GCOV_DEFAULT OFF)
option(ENABLE_GCOV "Enable integration with gcov, a code coverage program" ${ENABLE_GCOV_DEFAULT})

if (ENABLE_GCOV)
    if (NOT HAVE_GCOV)
    message (FATAL_ERROR
         "ENABLE_GCOV option requested but gcov library is not found")
    endif()

    add_compile_flags("C;CXX"
        "-fprofile-arcs"
        "-ftest-coverage"
    )

    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fprofile-arcs")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ftest-coverage")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fprofile-arcs")
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -ftest-coverage")

   # add_library(gcov SHARED IMPORTED)
endif()

if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(ENABLE_GPROF_DEFAULT ON)
else()
    set(ENABLE_GPROF_DEFAULT OFF)
endif()
option(ENABLE_GPROF "Enable integration with gprof, a performance analyzing tool" ${GPROF_DEFAULT})

if (ENABLE_GPROF)
    add_compile_flags("C;CXX" "-pg")
endif()

option(ENABLE_VALGRIND "Enable integration with valgrind, a memory analyzing tool" OFF)
if (ENABLE_VALGRIND)
    add_definitions(-UNVALGRIND)
else()
    add_definitions(-DNVALGRIND=1)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer, a fast memory error detector based on compiler instrumentation" OFF)
if (ENABLE_ASAN)
    if (CMAKE_COMPILER_IS_GNUCC)
        message(FATAL_ERROR
            "\n"
            " Tarantool does not support GCC's AddressSanitizer. Use clang:\n"
            " $ git clean -xfd; git submodule foreach --recursive git clean -xfd\n"
            " $ CC=clang CXX=clang++ cmake . <...> -DENABLE_ASAN=ON && make -j\n"
            "\n")
    endif()

    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/asan/asan.supp")
    check_c_source_compiles("int main(void) {
        #include <sanitizer/asan_interface.h>
        void *x;
	    __sanitizer_finish_switch_fiber(x);
        return 0;
        }" ASAN_INTERFACE_OLD)
    check_c_source_compiles("int main(void) {
        #include <sanitizer/asan_interface.h>
        void *x;
	    __sanitizer_finish_switch_fiber(x, 0, 0);
        return 0;
    }" ASAN_INTERFACE_NEW)
    set(CMAKE_REQUIRED_FLAGS "")

    if (ASAN_INTERFACE_OLD)
        add_definitions(-DASAN_INTERFACE_OLD=1)
    elseif (ASAN_INTERFACE_NEW)
        add_definitions(-UASAN_INTERFACE_OLD)
    else()
        message(FATAL_ERROR "Cannot enable AddressSanitizer")
    endif()

    add_compile_flags("C;CXX" -fsanitize=address -fsanitize-blacklist=${CMAKE_SOURCE_DIR}/asan/asan.supp)
endif()

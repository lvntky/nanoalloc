cmake_minimum_required(VERSION 3.30)

project(na_alloc
    VERSION 0.1.0
    LANGUAGES C
)

# -----------------------------
# Prefer clang (but do not force)
# -----------------------------
if(NOT DEFINED CMAKE_C_COMPILER)
    find_program(CLANG_BIN clang)
    if(CLANG_BIN)
        message(STATUS "Clang found: ${CLANG_BIN} — using it")
        set(CMAKE_C_COMPILER ${CLANG_BIN} CACHE STRING "" FORCE)
    else()
        message(STATUS "Clang not found — using default compiler")
    endif()
endif()

# -----------------------------
# C standard
# -----------------------------
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# -----------------------------
# Options
# -----------------------------
option(ENABLE_TESTING "Enable tests" ON)
option(ENABLE_SANITIZERS "Enable ASAN/UBSAN" OFF)

# -----------------------------
# Global compile flags
# -----------------------------
add_compile_options(
    -O2
    -g
    -Wall
    -Wextra
    -Wpedantic
    -fPIC
)

# Sanitizers (optional)
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined)
    add_link_options(-fsanitize=address,undefined)
endif()

# -----------------------------
# Source files (wildcard)
# -----------------------------
file(GLOB SRC CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.c
)

# -----------------------------
# Shared library (LD_PRELOAD)
# -----------------------------
add_library(na_alloc SHARED ${SRC})

target_include_directories(na_alloc PRIVATE include)

target_link_libraries(na_alloc
    dl
    pthread
)

set_target_properties(na_alloc PROPERTIES
    OUTPUT_NAME "na_alloc"
)

# -----------------------------
# Testing
# -----------------------------
if(ENABLE_TESTING)

    enable_testing()
    include(FetchContent)

    # Fetch Criterion
    FetchContent_Declare(
        criterion
        GIT_REPOSITORY https://github.com/Snaipe/Criterion.git
        GIT_TAG v2.4.2
    )

    FetchContent_MakeAvailable(criterion)

    # -------------------------
    # Auto-discover unit tests
    # -------------------------
    #file(GLOB TEST_FILES CONFIGURE_DEPENDS
        # ${CMAKE_SOURCE_DIR}/tests/test_*.c
   # )

    #foreach(test_file ${TEST_FILES})
     #   get_filename_component(test_name ${test_file} NAME_WE)

#        add_executable(${test_name}
 #           ${test_file}
  #          src/core.c
   #     )

    #    target_include_directories(${test_name} PRIVATE include)

#        target_link_libraries(${test_name} criterion)

 #       add_test(NAME ${test_name} COMMAND ${test_name})
  #  endforeach()

    # -------------------------
    # Preload integration test
    # -------------------------
#    add_executable(test_preload tests/test_preload.c)

 #   add_test(NAME preload_test
  #      COMMAND ${CMAKE_COMMAND} -E env
   #         NA_STATS=1
     #       LD_PRELOAD=$<TARGET_FILE:na_alloc>
    #        $<TARGET_FILE:test_preload>
    #)

    # -------------------------
    # Smoke tests
    # -------------------------
    add_test(NAME smoke_ls
        COMMAND ${CMAKE_COMMAND} -E env
            NA_STATS=1
            LD_PRELOAD=$<TARGET_FILE:na_alloc>
            ls /usr/bin
    )

    add_test(NAME smoke_python
        COMMAND ${CMAKE_COMMAND} -E env
            NA_STATS=1
            LD_PRELOAD=$<TARGET_FILE:na_alloc>
            python3 -c "import os; print('python ok, pid=', os.getpid())"
    )

endif()

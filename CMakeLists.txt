cmake_minimum_required (VERSION 3.23)
project(lua-cts)

include(CTest)

if (BUILD_TESTING)
    find_package(doctest 2.4.6 REQUIRED)

    add_library(DoctestIntegration STATIC
        tests/doctest.cpp
        )
    target_include_directories(DoctestIntegration PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/tests/ ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(DoctestIntegration doctest::doctest)
    target_compile_definitions(DoctestIntegration PUBLIC DOCTEST_CONFIG_SUPER_FAST_ASSERTS)

    function(lua_cts_test name)
        set(TESTNAME test_${name})

        add_executable(${TESTNAME}
            tests/${name}.cpp
            )
        target_link_libraries(test_${name} DoctestIntegration)

        add_test(${TESTNAME} ${TESTNAME})
    endfunction()
endif()

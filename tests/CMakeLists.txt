 # ================================================================
 # Author   : German Mundinger
 # Date     : 2023
 # ================================================================

FetchContent_Declare(
    google_tests
    SOURCE_DIR      ${PROJECT_SOURCE_DIR}/external/google_tests
    GIT_REPOSITORY  https://github.com/google/googletest.git
    GIT_TAG         v1.14.0
)
FetchContent_GetProperties(google_tests)
if(NOT google_tests_POPULATED)
    FetchContent_Populate(google_tests)
endif()

add_subdirectory(${google_tests_SOURCE_DIR} ${google_tests_BINARY_DIR})

add_executable(tests "")
target_sources(tests
    PRIVATE
        src/devices/mcp23017_expander.test.cpp
)
target_compile_features(tests
    PRIVATE
        cxx_std_17
)
set_target_properties(tests
    PROPERTIES
        CXX_STANDARD_REQUIRED ON
        CXX_EXTENSIONS OFF
)
target_link_libraries(tests
    PRIVATE
        GTest::gmock
        GTest::gmock_main
        blackpill_under_testing
)

# Setup tests scanning
include(GoogleTest)
gtest_discover_tests(tests
    DISCOVERY_TIMEOUT 10
)
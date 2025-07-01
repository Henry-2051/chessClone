cmake_minimum_required(VERSION 3.16)
# (The minimum version declaration here is optional if already set at the top-level.)

# Create an executable for the unit tests.
add_executable(unit_tests
  test_main.cpp
  # Add additional test source files below if necessary
  # test_module1.cpp
  # test_module2.cpp
)

# Include the engine headers.
target_include_directories(unit_tests PRIVATE
  "${CMAKE_SOURCE_DIR}/src")

# Define a compile symbol for unit-test-specific code.
target_compile_definitions(unit_tests PRIVATE UNIT_TEST_BUILD)

# Link Catch2 and, if needed, the SFML/OpenGL libraries.
target_link_libraries(unit_tests PRIVATE
  Catch2::Catch2WithMain
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
  GL
)

# Register the test executable with CTest using the Catch2 helper.
include(CTest)
include(Catch)
catch_discover_tests(unit_tests)

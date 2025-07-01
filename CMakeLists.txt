cmake_minimum_required(VERSION 3.16)
project(chess_clone VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard globally.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set a default build type if none is specified.
if(NOT CMAKE_BUILD_TYPE)
  message(STATUS "No build type specified, defaulting to Debug")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug, Release)" FORCE)
endif()

# Common compile options for Unix-like systems.
if(UNIX)
  add_compile_options(-Wall -pedantic-errors)
endif()

# --------------------------------------------------------------------
# Additional options for a Debug (testing) build.
if(CMAKE_BUILD_TYPE MATCHES Debug)
  message(STATUS "Configuring Debug build: enabling assertions and extra checks")
  add_compile_definitions(DEBUG_BUILD)
  if(UNIX)
    # Consider using -Og for debugging rather than -O2.
    add_compile_options(-O2)
  endif()
endif()

# --------------------------------------------------------------------
# Set up Catch2 via FetchContent.
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.4.0   # use the desired version tag
)
FetchContent_MakeAvailable(catch2)

# --------------------------------------------------------------------
# Unit Testing Setup in the main CMake file.
# (If you choose to have your tests here, you may remove add_subdirectory(tests))
# add_executable(unit_tests test_main.cpp)
# target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
# include(CTest)
# include(Catch)
# catch_discover_tests(unit_tests)

# Alternatively, if you prefer tests in a subdirectory:
option(ENABLE_UNIT_TESTS "Enable building unit tests" ON)
if(ENABLE_UNIT_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# --------------------------------------------------------------------
# Configure main executable target.
file(GLOB SRC_FILES
  "${CMAKE_SOURCE_DIR}/src/*.cpp")
  # "${CMAKE_SOURCE_DIR}/src/imgui/*.cpp"
  # "${CMAKE_SOURCE_DIR}/src/imgui-sfml/*.cpp")

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/src")
  # "${CMAKE_SOURCE_DIR}/src/imgui"
  # "${CMAKE_SOURCE_DIR}/src/imgui-sfml")

set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_link_libraries(${PROJECT_NAME} PRIVATE
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
  GL
  )

# --------------------------------------------------------------------
# Custom Target to Build and Run the Main Application.
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
  COMMAND "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Building and running ${PROJECT_NAME}")

cmake_minimum_required(VERSION 3.16)
project(chess_clone VERSION 1.0 LANGUAGES CXX)

# Set the C++ standard globally.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS="pg")

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

# disable for now until we have the library structure figured out

# option(ENABLE_UNIT_TESTS "Enable building unit tests" ON)
# if(ENABLE_UNIT_TESTS)
#   enable_testing()
#   add_subdirectory(tests)
# endif()

# --------------------------------------------------------------------
# Configure main executable target.

add_executable(gui_chess_game 
    ./src-gui-game/main.cpp
    ./src-gui-game/imgui-sfml/imgui-SFML.cpp
)

target_include_directories(gui_chess_game PRIVATE
  "${CMAKE_SOURCE_DIR}/src-gui-game"
  "${CMAKE_SOURCE_DIR}/src-lib"
  "${CMAKE_SOURCE_DIR}/shared-src-header-libs"
  "${CMAKE_SOURCE_DIR}/src-gui-game/imgui-sfml")

add_executable(pertree_debugger
    ./src-perftree-debugger/main.cpp
)

target_include_directories(pertree_debugger PRIVATE
  "${CMAKE_SOURCE_DIR}/src-perftree-debugger"
  "${CMAKE_SOURCE_DIR}/src-lib"
  "${CMAKE_SOURCE_DIR}/shared-src-header-libs"
  "${CMAKE_SOURCE_DIR}/src-gui-game/imgui-sfml")

add_library(chessBoard ./src-lib/chessBoard.cpp)

target_include_directories(chessBoard PRIVATE
    "${CMAKE_SOURCE_DIR}/src-lib/"
    "${CMAKE_SOURCE_DIR}/shared-src-header-libs/"
)

add_library(timer ./src-lib/timer.cpp)

target_include_directories(timer PRIVATE
    "${CMAKE_SOURCE_DIR}/src-lib/"
    "${CMAKE_SOURCE_DIR}/shared-src-header-libs/"
)

add_library(pieceMovements ./src-lib/pieceMovements.cpp)

target_include_directories(pieceMovements PRIVATE
    "${CMAKE_SOURCE_DIR}/src-lib/"
    "${CMAKE_SOURCE_DIR}/shared-src-header-libs/"
)

set_target_properties(gui_chess_game PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set_target_properties(pertree_debugger PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_link_libraries(gui_chess_game PRIVATE
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
  imgui
  GL
  chessBoard
  pieceMovements
  timer
  )

target_link_libraries(pertree_debugger PRIVATE
  chessBoard
  pieceMovements
  timer
)

# --------------------------------------------------------------------
# Custom Target to Build and Run the Main Application.
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
  COMMAND "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Building and running ${PROJECT_NAME}")

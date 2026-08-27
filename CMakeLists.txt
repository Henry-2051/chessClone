cmake_minimum_required(VERSION 3.31)
project(whatnotChess VERSION 1.0 LANGUAGES CXX)

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

# # --------------------------------------------------------------------
# # Additional options for a Debug (testing) build.
# if(CMAKE_BUILD_TYPE MATCHES Debug)
#   message(STATUS "Configuring Debug build: enabling assertions and extra checks")
#   add_compile_definitions(DEBUG_BUILD)
#   if(UNIX)
#     # Consider using -Og for debugging rather than -O2.
#     add_compile_options(-O2)
#   endif()
# endif()

# --------------------------------------------------------------------
# Set up Catch2 via FetchContent.
# include(FetchContent)
# FetchContent_Declare(
#   catch2
#   GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#   GIT_TAG v3.4.0   # use the desired version tag
# )
# FetchContent_MakeAvailable(catch2)

add_library(misc_libraries INTERFACE)

target_include_directories(misc_libraries INTERFACE
    "${CMAKE_SOURCE_DIR}/src/misc-lib"
    "${CMAKE_SOURCE_DIR}/src/shared-src-header-libs"
    "${CMAKE_SOURCE_DIR}/src/doctest" 
)

add_library(imgui_sfml INTERFACE)

target_include_directories(imgui_sfml INTERFACE
  "${CMAKE_SOURCE_DIR}/src/gui/imgui-sfml"
)

add_executable(whatnotGui 
    ./src/gui/main.cpp
    ./src/gui/imgui-sfml/imgui-SFML.cpp
)


target_include_directories(whatnotGui PRIVATE
  "${CMAKE_SOURCE_DIR}/src/gui"
  "${CMAKE_SOURCE_DIR}/src/engine"
)

add_executable(perftree_debugger
    ./src/perftree-debugger/main.cpp
)

target_include_directories(perftree_debugger PRIVATE
  "${CMAKE_SOURCE_DIR}/src/perftree-debugger"
)

add_executable(uciEngine
    ./src/engine/main.cpp
)

target_include_directories(uciEngine PRIVATE
  "${CMAKE_SOURCE_DIR}/src/engine"
)


target_include_directories(perftree_debugger PRIVATE
  "${CMAKE_SOURCE_DIR}/src/perftree-debugger"
)

add_library(chessBoard ./src/misc-lib/chessBoard.cpp)


add_library(timer ./src/misc-lib/timer.cpp)


add_library(pieceMovements ./src/misc-lib/pieceMovements.cpp)

add_library(transpositionTable ./src/engine/transpositionTable.cpp)

add_library(engine ./src/engine/engine.cpp)

target_include_directories(engine PRIVATE
  "${CMAKE_SOURCE_DIR}/src/engine"
)

add_executable(test_runner
    ./src/tests/main.cpp
)

add_library(search ./src/engine/search.cpp)

target_include_directories(search PRIVATE
  "${CMAKE_SOURCE_DIR}/src/engine"
)

target_include_directories(test_runner PRIVATE
    "${CMAKE_SOURCE_DIR}/src/tests" 
    "${CMAKE_SOURCE_DIR}/src/doctest" 
    "${CMAKE_SOURCE_DIR}/src/perftree-debugger"
)

set_target_properties(whatnotGui PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set_target_properties(perftree_debugger PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set_target_properties(uciEngine PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

set_target_properties(test_runner PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

target_link_libraries(whatnotGui PRIVATE
  sfml-graphics
  sfml-window
  sfml-system
  sfml-audio
  imgui
  GL
  chessBoard
  pieceMovements
  timer
  misc_libraries
  engine
)

target_link_libraries(transpositionTable PRIVATE misc_libraries)

target_link_libraries(chessBoard PRIVATE misc_libraries)

target_link_libraries(pieceMovements PRIVATE misc_libraries)

target_link_libraries(perftree_debugger PRIVATE
  chessBoard
  pieceMovements
  timer
  misc_libraries
)

target_link_libraries(uciEngine PRIVATE
  chessBoard
  misc_libraries
  engine
)

target_link_libraries(search PRIVATE
  misc_libraries
  transpositionTable
  pieceMovements
  chessBoard
)

target_link_libraries(engine PRIVATE
  misc_libraries
  transpositionTable
  pieceMovements
  chessBoard
  search
)

target_link_libraries(test_runner PRIVATE misc_libraries pieceMovements chessBoard)

# --------------------------------------------------------------------
# Custom Target to Build and Run the Main Application.
add_custom_target(run
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${PROJECT_NAME}
  COMMAND "${CMAKE_SOURCE_DIR}/bin/${PROJECT_NAME}"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Building and running ${PROJECT_NAME}")

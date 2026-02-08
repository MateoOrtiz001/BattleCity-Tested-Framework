cmake_minimum_required(VERSION 3.5)
project(BattleCity LANGUAGES CXX)

# CMake 4.x removed compatibility with CMake < 3.5.
# Some third-party deps (e.g., raylib 5.0) still declare cmake_minimum_required(VERSION 3.0),
# so we set a minimum policy version to allow configuring them.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- MSVC runtime (fix link conflicts with raylib) ----
# The warnings/errors you saw (LNK4098: MSVCRTD conflicts, many __imp_* unresolved)
# typically come from mixing CRT runtimes (/MTd vs /MDd, etc.).
# Force a single runtime for everything in this build.
if (MSVC)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
endif()

# ---- Raylib (FetchContent) ----
include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG 5.0
)
FetchContent_MakeAvailable(raylib)

# ---- Engine ----
file(GLOB_RECURSE ENGINE_SOURCES
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine/src/*.cpp"
)

add_library(BattleCityEngine STATIC ${ENGINE_SOURCES})
target_include_directories(BattleCityEngine PUBLIC
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine"
)

# ---- Orchestrator (AI / bots) ----
file(GLOB_RECURSE ORCHESTRATOR_SOURCES
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Orchestrator/src/*.cpp"
)

add_library(BattleCityOrchestrator STATIC ${ORCHESTRATOR_SOURCES})
target_include_directories(BattleCityOrchestrator PUBLIC
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Orchestrator"
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine"
)
target_link_libraries(BattleCityOrchestrator PUBLIC BattleCityEngine)

# ---- GUI (raylib) ----
file(GLOB_RECURSE GUI_SOURCES
  "${CMAKE_SOURCE_DIR}/src/BattleCity.GUI/src/*.cpp"
)

add_executable(BattleCityGUI ${GUI_SOURCES})
target_include_directories(BattleCityGUI PRIVATE
  "${CMAKE_SOURCE_DIR}/src/BattleCity.GUI/src"
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine"
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Orchestrator"
)

target_link_libraries(BattleCityGUI PRIVATE BattleCityOrchestrator BattleCityEngine raylib)

# Windows: avoid console popup vs subsystem left as default (console) for logs.

cmake_minimum_required(VERSION 3.5)
project(BattleCity LANGUAGES CXX)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- MSVC runtime ----
if (MSVC)
  cmake_policy(SET CMP0091 NEW)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" CACHE STRING "" FORCE)
endif()

# ---- Engine (biblioteca estática) ----
file(GLOB_RECURSE ENGINE_SOURCES
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine/src/*.cpp"
)

# Runner.cpp depende de Orchestrator (ScriptedEnemyAgent), así que se
# compila en ese target para evitar dependencia circular.

add_library(BattleCityEngine STATIC ${ENGINE_SOURCES})
target_include_directories(BattleCityEngine PUBLIC
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine"
)

# ---- Orchestrator (agentes/IA) ----
file(GLOB_RECURSE ORCHESTRATOR_SOURCES
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Orchestrator/src/*.cpp"
)

# Runner.cpp se compila aquí porque depende de ScriptedEnemyAgent (Orchestrator)
list(APPEND ORCHESTRATOR_SOURCES
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine/simulator/Runner.cpp"
)

add_library(BattleCityOrchestrator STATIC ${ORCHESTRATOR_SOURCES})
target_include_directories(BattleCityOrchestrator PUBLIC
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Orchestrator"
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine"
)
target_link_libraries(BattleCityOrchestrator PUBLIC BattleCityEngine)

# ---- Headless Runner (ejecutable sin GUI, sin raylib) ----
add_executable(BattleCityHeadless
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Headless/main.cpp"
)
target_include_directories(BattleCityHeadless PRIVATE
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Engine"
  "${CMAKE_SOURCE_DIR}/src/BattleCity.Orchestrator"
)
target_link_libraries(BattleCityHeadless PRIVATE BattleCityOrchestrator BattleCityEngine)
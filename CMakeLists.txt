cmake_minimum_required(VERSION 3.10)
project(syncpad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/core)

# core sources
set(CORE_SOURCES
  core/transport.cpp
)

# CLI
add_executable(syncpad-cli ui_cli/cli_main.cpp ${CORE_SOURCES})
target_link_libraries(syncpad-cli PRIVATE pthread)

# GUI (left unchanged from M0 if you have it)
find_package(FLTK QUIET)
if (FLTK_FOUND)
  add_executable(syncpad-gui ui_gui/gui_main.cpp ${CORE_SOURCES})
  target_include_directories(syncpad-gui PRIVATE ${FLTK_INCLUDE_DIR})
  target_link_libraries(syncpad-gui PRIVATE fltk pthread)
endif()

# Tests
# add_executable(test-doc tests/test_doc.cpp core/document.cpp core/crc32.cpp)

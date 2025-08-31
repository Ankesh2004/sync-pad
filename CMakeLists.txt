cmake_minimum_required(VERSION 3.10)
project(syncpad LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Core include
include_directories(${CMAKE_SOURCE_DIR}/core)

# CLI executable
add_executable(syncpad-cli ui_cli/cli_main.cpp core/common.hpp)

# GUI executable using FLTK
find_package(FLTK REQUIRED)
add_executable(syncpad-gui ui_gui/gui_main.cpp core/common.hpp)
target_link_libraries(syncpad-gui PRIVATE fltk)

# Tests
add_executable(test-doc tests/test_doc.cpp core/document.cpp core/crc32.cpp)

cmake_minimum_required(VERSION 3.25)
project(eigen_fun LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# Optional sanitizers: -DEF_SANITIZE=address  or  -DEF_SANITIZE=thread
set(EF_SANITIZE "" CACHE STRING "Sanitizer to enable (address, thread, undefined, or empty)")

include(cmake/AppleLibcxxWorkaround.cmake)
include(FetchContent)
include(cmake/Dependencies.cmake)

# ---------------------------------------------------------------------------
# Library with the sample problem implementations
# ---------------------------------------------------------------------------
add_library(eigen_fun
  src/linalg.cpp
  src/parallel.cpp
)
target_include_directories(eigen_fun PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(eigen_fun PUBLIC Eigen3::Eigen Threads::Threads)
target_compile_options(eigen_fun PRIVATE -Wall -Wextra -Wpedantic)

if(EF_SANITIZE)
  target_compile_options(eigen_fun PUBLIC -fsanitize=${EF_SANITIZE} -fno-omit-frame-pointer)
  target_link_options(eigen_fun PUBLIC -fsanitize=${EF_SANITIZE})
endif()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)

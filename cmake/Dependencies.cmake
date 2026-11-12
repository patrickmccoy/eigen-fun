# Threads
find_package(Threads REQUIRED)

# Eigen: use a system install if one exists (e.g. `brew install eigen`),
# otherwise download the headers. Eigen is header-only, so we only need the
# source tree -- SOURCE_SUBDIR points at a directory with no CMakeLists.txt so
# FetchContent_MakeAvailable does not configure Eigen's own (large) build.
find_package(Eigen3 3.4 QUIET NO_MODULE)
if(NOT TARGET Eigen3::Eigen)
  message(STATUS "Eigen3 not found on system; fetching Eigen 3.4.0")
  FetchContent_Declare(eigen
    URL https://gitlab.com/libeigen/eigen/-/archive/3.4.0/eigen-3.4.0.tar.gz
    URL_HASH SHA256=8586084f71f9bde545ee7fa6d00288b264a2b7ac3607b974e54d13e7162c1c72
    SOURCE_SUBDIR do-not-configure
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  FetchContent_MakeAvailable(eigen)
  add_library(Eigen3::Eigen INTERFACE IMPORTED GLOBAL)
  # SYSTEM silences warnings coming from Eigen's headers.
  target_include_directories(Eigen3::Eigen SYSTEM INTERFACE ${eigen_SOURCE_DIR})
else()
  message(STATUS "Using system Eigen3 ${Eigen3_VERSION}")
endif()

# GoogleTest
FetchContent_Declare(googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.tar.gz
  URL_HASH SHA256=7b42b4d6ed48810c5362c265a17faebe90dc2373c885e5216439d37927f02926
  SYSTEM
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

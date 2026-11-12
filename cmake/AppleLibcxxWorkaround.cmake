# Some Command Line Tools installs ship an almost-empty
# /Library/Developer/CommandLineTools/usr/include/c++/v1 (only `__cxx_version`),
# which shadows the SDK's real libc++ headers so even `#include <cmath>` fails.
# Detect that and point the compiler at the SDK's libc++ headers instead.
# The proper fix is to reinstall the CLT:
#   sudo rm -rf /Library/Developer/CommandLineTools && xcode-select --install
if(NOT APPLE)
  return()
endif()

include(CheckCXXSourceCompiles)
check_cxx_source_compiles("#include <cmath>\nint main() { return 0; }" EF_STDLIB_HEADERS_OK)
if(EF_STDLIB_HEADERS_OK)
  return()
endif()

set(_sdk "${CMAKE_OSX_SYSROOT}")
if(NOT _sdk)
  execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE _sdk OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

if(EXISTS "${_sdk}/usr/include/c++/v1/cmath")
  message(WARNING
    "libc++ headers not found on the default search path (broken Command Line Tools?). "
    "Using ${_sdk}/usr/include/c++/v1 instead. Reinstall the CLT to remove this workaround.")
  add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-nostdinc++>
                      $<$<COMPILE_LANGUAGE:CXX>:-isystem${_sdk}/usr/include/c++/v1>)
else()
  message(FATAL_ERROR "C++ standard library headers not found; check your toolchain installation.")
endif()

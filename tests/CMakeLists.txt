include(GoogleTest)

# One test executable per domain so each can be run (and sanitized) separately.
function(ef_add_test_suite name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE eigen_fun GTest::gtest_main)
  target_compile_options(${name} PRIVATE -Wall -Wextra)
  gtest_discover_tests(${name} PROPERTIES LABELS ${name} DISCOVERY_TIMEOUT 30)
endfunction()

ef_add_test_suite(eigen_tests
  eigen/basics_test.cpp
  eigen/linalg_test.cpp
)

ef_add_test_suite(move_tests
  move/tracked_test.cpp
  move/buffer_test.cpp
  move/eigen_move_test.cpp
)

ef_add_test_suite(threading_tests
  threading/queue_test.cpp
  threading/thread_pool_test.cpp
  threading/parallel_test.cpp
)

# eigen-fun

A playground for the [Eigen](https://eigen.tuxfamily.org) linear-algebra library,
C++ move semantics, and multi-threading. Every example is a
[GoogleTest](https://github.com/google/googletest) test, so "running an example"
means running its test.

## Build and run

Requires CMake ≥ 3.25 and a C++20 compiler. Eigen 3.4 and GoogleTest are fetched
automatically. If Eigen is installed on your system (`brew install eigen`), that copy is used instead.

```sh
cmake --preset default        # configure into ./build
cmake --build --preset default
ctest --preset default        # run everything

# Run one domain, or one test, directly:
./build/tests/eigen_tests
./build/tests/move_tests --gtest_filter='TrackedTest.*'
ctest --test-dir build -L threading_tests

# Sanitizer builds (the threading and Rule-of-Five code is clean under both):
cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan
cmake --preset asan && cmake --build --preset asan && ctest --preset asan
```

## Layout

```
include/eigen_fun/   headers (move + threading code is header-only templates)
src/                 linalg.cpp, parallel.cpp
tests/eigen/         Eigen API learning tests + sample problems
tests/move/          move-semantics experiments
tests/threading/     concurrency primitives + parallel algorithms
cmake/               dependency fetching, toolchain workaround
```

## Sample problems

### Eigen (`linalg.hpp`, `tests/eigen/`)
| Problem | Eigen features exercised |
|---|---|
| Polynomial least-squares fit | Vandermonde construction, `colPivHouseholderQr` |
| Kabsch rigid alignment of point clouds | broadcasting, `JacobiSVD`, `AngleAxis`, reflection guard |
| Dominant eigenpair by power iteration | checked against `SelfAdjointEigenSolver` |
| Markov-chain stationary distribution | `fullPivLu` on a rank-deficient system |
| 1-D Poisson equation | `SparseMatrix`, triplet assembly, `SimplicialLDLT`, O(h²) convergence check |
| `basics_test.cpp` | storage order, blocks, array vs matrix, `Map`, aliasing/`noalias`, lazy `auto` expressions |

### Move semantics (`tracked.hpp`, `buffer.hpp`, `tests/move/`)
- `Tracked` counts every ctor/copy/move/dtor, so tests can assert exact costs:
  `std::move` is only a cast, const objects get copied, guaranteed elision vs. NRVO,
  the pessimizing `return std::move(x)`, sink-by-value, perfect forwarding,
  `emplace_back` vs. `push_back`.
- **`std::vector` only moves on reallocation when the move ctor is `noexcept`.** A test shows it copying otherwise.
- `Buffer<T>`: a hand-written Rule-of-Five owner with copy-and-swap.
- Eigen: dynamic matrices move by stealing the pointer; fixed-size ones (`Matrix4d`) just copy.

### Multi-threading (`thread_safe_queue.hpp`, `thread_pool.hpp`, `parallel.hpp`, `tests/threading/`)
- `ThreadSafeQueue<T>`: mutex + condition variable, close/drain semantics, MPMC stress test.
- `ThreadPool`: `std::jthread` workers, `submit()` returns `std::future`, handles move-only callables, propagates exceptions.
- Parallel algorithms: `std::async` reduction, Monte Carlo π with per-thread RNGs,
  row-blocked Eigen matrix multiply on the pool, per-thread histograms merged at the end.
- Atomics vs. mutex, `std::latch`, and `std::stop_token` cooperative cancellation.

## Adding an example

Drop a `*_test.cpp` into the matching `tests/<domain>/` folder, add it to the
`ef_add_test_suite(...)` list in `tests/CMakeLists.txt`, and put any reusable
code in `include/eigen_fun/` (plus `src/` if it isn't a template).

## Toolchain note (macOS)

Some Command Line Tools installs leave an almost-empty
`/Library/Developer/CommandLineTools/usr/include/c++/v1`, which hides the
SDK's libc++ headers (`'cmath' file not found`). `cmake/AppleLibcxxWorkaround.cmake`
detects this and points at the SDK's headers, printing a CMake warning. The proper fix:

```sh
sudo rm -rf /Library/Developer/CommandLineTools && xcode-select --install
```

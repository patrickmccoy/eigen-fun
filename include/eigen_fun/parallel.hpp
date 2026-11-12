#pragma once
// Sample data-parallel problems.

#include "eigen_fun/thread_pool.hpp"

#include <Eigen/Dense>
#include <cstdint>
#include <span>

namespace ef::threading {

// --- Problem 1: parallel reduction with std::async --------------------------
double parallelSum(std::span<const double> values, unsigned numTasks);

// --- Problem 2: Monte Carlo estimate of pi ----------------------------------
// Each thread owns its RNG (seeded from `seed` + thread index), so the result is
// deterministic for a given (samples, numThreads, seed) and there's no sharing.
double monteCarloPi(std::uint64_t samples, unsigned numThreads, std::uint64_t seed = 42);

// --- Problem 3: row-blocked matrix multiply on a thread pool -----------------
// Each task computes a disjoint horizontal band of C = A * B, so no locking is
// needed: threads never write to the same memory.
Eigen::MatrixXd parallelMatMul(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, ThreadPool& pool);

// --- Problem 4: parallel histogram -------------------------------------------
// Counts how many values fall in each of `bins` equal-width bins over [lo, hi).
// Uses per-thread local histograms merged at the end (avoids contention).
Eigen::VectorXi parallelHistogram(std::span<const double> values, double lo, double hi, int bins,
                                  unsigned numThreads);

}  // namespace ef::threading

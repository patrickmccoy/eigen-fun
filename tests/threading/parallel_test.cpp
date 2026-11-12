#include "eigen_fun/parallel.hpp"

#include <gtest/gtest.h>
#include <chrono>
#include <cmath>
#include <iostream>
#include <numbers>
#include <numeric>
#include <random>
#include <vector>

using namespace ef::threading;

TEST(ParallelSum, MatchesSerialSumForVariousTaskCounts) {
    std::vector<double> v(1'000'003);
    std::iota(v.begin(), v.end(), 1.0);
    const double expected = 1'000'003.0 * 1'000'004.0 / 2.0;
    for (unsigned tasks : {1u, 2u, 3u, 7u, 16u})
        EXPECT_DOUBLE_EQ(parallelSum(v, tasks), expected) << tasks << " tasks";
}

TEST(ParallelSum, HandlesMoreTasksThanElements) {
    std::vector<double> v = {1.0, 2.0};
    EXPECT_DOUBLE_EQ(parallelSum(v, 8), 3.0);
    EXPECT_DOUBLE_EQ(parallelSum({}, 4), 0.0);
}

TEST(MonteCarloPi, ConvergesAndIsDeterministicPerSeed) {
    const double est = monteCarloPi(2'000'000, 4);
    EXPECT_NEAR(est, std::numbers::pi, 0.01);
    EXPECT_DOUBLE_EQ(est, monteCarloPi(2'000'000, 4));  // same seed, same threads
}

TEST(ParallelMatMul, MatchesEigenSerialProduct) {
    ThreadPool pool(4);
    Eigen::MatrixXd A = Eigen::MatrixXd::Random(203, 150);
    Eigen::MatrixXd B = Eigen::MatrixXd::Random(150, 97);
    EXPECT_TRUE(parallelMatMul(A, B, pool).isApprox(A * B, 1e-12));
}

TEST(ParallelMatMul, FewerRowsThanThreads) {
    ThreadPool pool(8);
    Eigen::MatrixXd A = Eigen::MatrixXd::Random(3, 5);
    Eigen::MatrixXd B = Eigen::MatrixXd::Random(5, 4);
    EXPECT_TRUE(parallelMatMul(A, B, pool).isApprox(A * B));
}

TEST(ParallelMatMul, ReportsSpeedup) {
    // Informational only -- timing assertions make flaky tests.
    ThreadPool pool;
    Eigen::MatrixXd A = Eigen::MatrixXd::Random(800, 800);
    Eigen::MatrixXd B = Eigen::MatrixXd::Random(800, 800);
    using clock = std::chrono::steady_clock;

    auto t0 = clock::now();
    Eigen::MatrixXd serial = A * B;
    auto t1 = clock::now();
    Eigen::MatrixXd parallel = parallelMatMul(A, B, pool);
    auto t2 = clock::now();

    const double ms1 = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const double ms2 = std::chrono::duration<double, std::milli>(t2 - t1).count();
    std::cout << "[ info     ] serial " << ms1 << " ms, " << pool.size() << " threads " << ms2
              << " ms, speedup x" << ms1 / ms2 << "\n";
    RecordProperty("speedup", std::to_string(ms1 / ms2));
    EXPECT_TRUE(parallel.isApprox(serial));
}

TEST(ParallelHistogram, MatchesSerialCount) {
    std::mt19937 rng(1);
    std::normal_distribution<double> nd(0.0, 1.0);
    std::vector<double> v(500'000);
    for (auto& x : v) x = nd(rng);

    const double lo = -3.0, hi = 3.0;
    const int bins = 12;
    Eigen::VectorXi serial = Eigen::VectorXi::Zero(bins);
    for (double x : v)
        if (x >= lo && x < hi) serial(static_cast<int>((x - lo) * bins / (hi - lo)))++;

    Eigen::VectorXi par = parallelHistogram(v, lo, hi, bins, 6);
    EXPECT_EQ(par, serial);
    // Symmetric distribution: the two central bins should be roughly equal.
    EXPECT_NEAR(par(5), par(6), 0.02 * par(5));
}

#include "eigen_fun/parallel.hpp"

#include <algorithm>
#include <future>
#include <numeric>
#include <random>
#include <thread>
#include <vector>

namespace ef::threading {

namespace {
// Split [0, n) into `parts` nearly equal half-open ranges.
std::pair<std::size_t, std::size_t> chunk(std::size_t n, std::size_t parts, std::size_t i) {
    const std::size_t base = n / parts, extra = n % parts;
    const std::size_t begin = i * base + std::min(i, extra);
    return {begin, begin + base + (i < extra ? 1 : 0)};
}
}  // namespace

double parallelSum(std::span<const double> values, unsigned numTasks) {
    numTasks = std::max(1u, numTasks);
    std::vector<std::future<double>> partials;
    partials.reserve(numTasks);
    for (unsigned i = 0; i < numTasks; ++i) {
        auto [b, e] = chunk(values.size(), numTasks, i);
        // std::launch::async forces a real thread (the default policy may defer).
        partials.push_back(std::async(std::launch::async, [sub = values.subspan(b, e - b)] {
            return std::accumulate(sub.begin(), sub.end(), 0.0);
        }));
    }
    double total = 0.0;
    for (auto& f : partials) total += f.get();
    return total;
}

double monteCarloPi(std::uint64_t samples, unsigned numThreads, std::uint64_t seed) {
    numThreads = std::max(1u, numThreads);
    std::vector<std::uint64_t> hits(numThreads, 0);
    {
        std::vector<std::jthread> threads;
        for (unsigned t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t] {
                auto [b, e] = chunk(samples, numThreads, t);
                std::mt19937_64 rng(seed + t);
                std::uniform_real_distribution<double> u(0.0, 1.0);
                std::uint64_t local = 0;  // accumulate locally to avoid false sharing
                for (auto i = b; i < e; ++i) {
                    const double x = u(rng), y = u(rng);
                    local += (x * x + y * y <= 1.0);
                }
                hits[t] = local;  // each thread writes only its own slot
            });
        }
    }  // jthreads join here
    const auto total = std::accumulate(hits.begin(), hits.end(), std::uint64_t{0});
    return 4.0 * static_cast<double>(total) / static_cast<double>(samples);
}

Eigen::MatrixXd parallelMatMul(const Eigen::MatrixXd& A, const Eigen::MatrixXd& B, ThreadPool& pool) {
    if (A.cols() != B.rows()) throw std::invalid_argument("parallelMatMul: dimension mismatch");
    Eigen::MatrixXd C(A.rows(), B.cols());
    const std::size_t parts = std::min<std::size_t>(pool.size(), static_cast<std::size_t>(A.rows()));

    std::vector<std::future<void>> done;
    for (std::size_t i = 0; i < parts; ++i) {
        auto [b, e] = chunk(static_cast<std::size_t>(A.rows()), parts, i);
        const auto r0 = static_cast<Eigen::Index>(b), n = static_cast<Eigen::Index>(e - b);
        done.push_back(pool.submit([&A, &B, &C, r0, n] {
            C.middleRows(r0, n).noalias() = A.middleRows(r0, n) * B;
        }));
    }
    for (auto& f : done) f.get();  // rethrows any exception from a worker
    return C;
}

Eigen::VectorXi parallelHistogram(std::span<const double> values, double lo, double hi, int bins,
                                  unsigned numThreads) {
    if (bins <= 0 || !(hi > lo)) throw std::invalid_argument("parallelHistogram: bad range");
    numThreads = std::max(1u, numThreads);
    std::vector<Eigen::VectorXi> locals(numThreads, Eigen::VectorXi::Zero(bins));
    {
        std::vector<std::jthread> threads;
        for (unsigned t = 0; t < numThreads; ++t) {
            threads.emplace_back([&, t] {
                auto [b, e] = chunk(values.size(), numThreads, t);
                Eigen::VectorXi& h = locals[t];
                const double scale = bins / (hi - lo);
                for (auto i = b; i < e; ++i) {
                    const double v = values[i];
                    if (v < lo || v >= hi) continue;
                    h(std::min(bins - 1, static_cast<int>((v - lo) * scale)))++;
                }
            });
        }
    }
    Eigen::VectorXi total = Eigen::VectorXi::Zero(bins);
    for (const auto& h : locals) total += h;
    return total;
}

}  // namespace ef::threading

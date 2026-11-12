#include "eigen_fun/linalg.hpp"

#include <Eigen/Geometry>
#include <gtest/gtest.h>
#include <cmath>
#include <numbers>

using namespace ef::linalg;
using Eigen::MatrixXd;
using Eigen::VectorXd;

// ---------------------------------------------------------------- polyfit ----
TEST(Polyfit, RecoversExactQuadratic) {
    VectorXd x = VectorXd::LinSpaced(20, -2.0, 3.0);
    VectorXd y = (1.5 - 2.0 * x.array() + 0.5 * x.array().square()).matrix();
    VectorXd c = polyfit(x, y, 2);
    EXPECT_TRUE(c.isApprox(Eigen::Vector3d(1.5, -2.0, 0.5), 1e-10)) << c.transpose();
    EXPECT_NEAR(polyval(c, 10.0), 1.5 - 20.0 + 50.0, 1e-8);
}

TEST(Polyfit, LeastSquaresLineThroughNoisyData) {
    // Symmetric +/- noise has zero mean, so the best-fit line is exact.
    VectorXd x = VectorXd::LinSpaced(100, 0.0, 10.0);
    VectorXd noise(100);
    for (int i = 0; i < 100; ++i) noise(i) = (i % 2 == 0) ? 0.1 : -0.1;
    VectorXd y = (3.0 + 2.0 * x.array()).matrix() + noise;
    VectorXd c = polyfit(x, y, 1);
    EXPECT_NEAR(c(0), 3.0, 0.02);
    EXPECT_NEAR(c(1), 2.0, 0.01);
}

TEST(Polyfit, RejectsTooFewPoints) {
    EXPECT_THROW(polyfit(VectorXd::Ones(2), VectorXd::Ones(2), 3), std::invalid_argument);
}

// ----------------------------------------------------------------- kabsch ----
TEST(Kabsch, RecoversKnownRotationAndTranslation) {
    const Eigen::Matrix3d R =
        Eigen::AngleAxisd(0.7, Eigen::Vector3d(1, 2, 3).normalized()).toRotationMatrix();
    const Eigen::Vector3d t(0.5, -1.0, 2.0);

    Eigen::Matrix3Xd src = Eigen::Matrix3Xd::Random(3, 25);
    Eigen::Matrix3Xd dst = (R * src).colwise() + t;

    RigidTransform est = kabsch(src, dst);
    EXPECT_TRUE(est.R.isApprox(R, 1e-10));
    EXPECT_TRUE(est.t.isApprox(t, 1e-10));
    EXPECT_NEAR(est.R.determinant(), 1.0, 1e-12);
}

TEST(Kabsch, NeverReturnsAReflection) {
    // dst is a mirror image of src: the best *proper* rotation has det +1.
    Eigen::Matrix3Xd src = Eigen::Matrix3Xd::Random(3, 10);
    Eigen::Matrix3Xd dst = Eigen::Vector3d(-1, 1, 1).asDiagonal() * src;
    EXPECT_NEAR(kabsch(src, dst).R.determinant(), 1.0, 1e-12);
}

// --------------------------------------------------------- power iteration ----
TEST(PowerIteration, MatchesSelfAdjointEigenSolver) {
    MatrixXd B = MatrixXd::Random(30, 30);
    MatrixXd A = B.transpose() * B;  // symmetric PSD -> real, non-negative spectrum

    EigenPair p = powerIteration(A);
    Eigen::SelfAdjointEigenSolver<MatrixXd> es(A);
    const double expected = es.eigenvalues().maxCoeff();  // sorted ascending

    EXPECT_NEAR(p.value, expected, 1e-8 * expected);
    // Check the defining property A v = lambda v.
    EXPECT_LT((A * p.vector - p.value * p.vector).norm(), 1e-4 * expected);
}

// ------------------------------------------------------------ markov chain ----
TEST(Markov, WeatherChainStationaryDistribution) {
    // States: sunny, cloudy, rainy.
    Eigen::Matrix3d P;
    P << 0.8, 0.15, 0.05,
         0.2, 0.6,  0.2,
         0.25, 0.25, 0.5;
    VectorXd pi = stationaryDistribution(P);
    EXPECT_NEAR(pi.sum(), 1.0, 1e-12);
    EXPECT_TRUE((pi.transpose() * P).transpose().isApprox(pi, 1e-12));
    EXPECT_TRUE((pi.array() > 0).all());

    // Cross-check: iterating the chain from any start converges to pi.
    Eigen::RowVector3d state(1, 0, 0);
    for (int i = 0; i < 200; ++i) state = state * P;
    EXPECT_TRUE(state.transpose().isApprox(pi, 1e-10));
}

// ------------------------------------------------------------ sparse poisson ----
TEST(Poisson1D, MatchesAnalyticSolution) {
    using std::numbers::pi;
    auto f = [](double x) { return pi * pi * std::sin(pi * x); };  // u = sin(pi x)

    auto maxError = [&](int n) {
        VectorXd u = solvePoisson1D(f, n);
        double err = 0.0;
        for (int i = 0; i < n; ++i) {
            double x = (i + 1.0) / (n + 1);
            err = std::max(err, std::abs(u(i) - std::sin(pi * x)));
        }
        return err;
    };

    const double e1 = maxError(100);
    const double e2 = maxError(200);
    EXPECT_LT(e1, 1e-4);
    // Second-order scheme: halving h should cut the error by ~4x.
    EXPECT_NEAR(e1 / e2, 4.0, 0.2);
}

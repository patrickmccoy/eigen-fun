#pragma once
// Sample linear-algebra problems solved with Eigen.

#include <Eigen/Dense>
#include <functional>

namespace ef::linalg {

// --- Problem 1: polynomial least-squares fit ---------------------------------
// Fit y ~= c0 + c1*x + ... + cd*x^d by building a Vandermonde matrix and
// solving the overdetermined system with a rank-revealing QR decomposition.
Eigen::VectorXd polyfit(const Eigen::VectorXd& x, const Eigen::VectorXd& y, int degree);

// Evaluate a polynomial (coefficients lowest order first) using Horner's rule.
double polyval(const Eigen::VectorXd& coeffs, double x);

// --- Problem 2: rigid point-cloud alignment (Kabsch algorithm) ----------------
// Find rotation R and translation t minimising sum ||R*src_i + t - dst_i||^2.
// Points are stored as columns of a 3xN matrix.
struct RigidTransform {
    Eigen::Matrix3d R;
    Eigen::Vector3d t;
};
RigidTransform kabsch(const Eigen::Matrix3Xd& src, const Eigen::Matrix3Xd& dst);

// --- Problem 3: dominant eigenpair by power iteration -------------------------
struct EigenPair {
    double value;
    Eigen::VectorXd vector;
    int iterations;
};
EigenPair powerIteration(const Eigen::MatrixXd& A, int maxIter = 10'000, double tol = 1e-12);

// --- Problem 4: stationary distribution of a Markov chain ---------------------
// P is row-stochastic (rows sum to 1). Returns pi with pi^T P = pi^T, sum(pi)=1.
Eigen::VectorXd stationaryDistribution(const Eigen::MatrixXd& P);

// --- Problem 5: 1-D Poisson equation with a sparse solver ---------------------
// Solve -u''(x) = f(x) on (0,1) with u(0)=u(1)=0 using second-order finite
// differences on n interior points. Returns u at x_i = i/(n+1), i=1..n.
Eigen::VectorXd solvePoisson1D(const std::function<double(double)>& f, int n);

}  // namespace ef::linalg

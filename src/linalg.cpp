#include "eigen_fun/linalg.hpp"

#include <Eigen/Sparse>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace ef::linalg {

Eigen::VectorXd polyfit(const Eigen::VectorXd& x, const Eigen::VectorXd& y, int degree) {
    if (x.size() != y.size()) throw std::invalid_argument("polyfit: x and y size mismatch");
    if (degree < 0 || x.size() <= degree) throw std::invalid_argument("polyfit: not enough points");

    // Vandermonde matrix: V(i, j) = x_i^j, built column by column.
    Eigen::MatrixXd V(x.size(), degree + 1);
    V.col(0).setOnes();
    for (int j = 1; j <= degree; ++j) V.col(j) = V.col(j - 1).cwiseProduct(x);

    // QR is numerically preferable to the normal equations (V^T V)^-1 V^T y.
    return V.colPivHouseholderQr().solve(y);
}

double polyval(const Eigen::VectorXd& coeffs, double x) {
    double acc = 0.0;
    for (Eigen::Index i = coeffs.size() - 1; i >= 0; --i) acc = acc * x + coeffs(i);
    return acc;
}

RigidTransform kabsch(const Eigen::Matrix3Xd& src, const Eigen::Matrix3Xd& dst) {
    if (src.cols() != dst.cols() || src.cols() < 3)
        throw std::invalid_argument("kabsch: need >= 3 corresponding points");

    const Eigen::Vector3d srcCentroid = src.rowwise().mean();
    const Eigen::Vector3d dstCentroid = dst.rowwise().mean();

    // Broadcasting: subtract the centroid from every column.
    const Eigen::Matrix3Xd S = src.colwise() - srcCentroid;
    const Eigen::Matrix3Xd D = dst.colwise() - dstCentroid;

    const Eigen::Matrix3d H = S * D.transpose();  // 3x3 cross-covariance
    Eigen::JacobiSVD<Eigen::Matrix3d> svd(H, Eigen::ComputeFullU | Eigen::ComputeFullV);
    const Eigen::Matrix3d& U = svd.matrixU();
    const Eigen::Matrix3d& V = svd.matrixV();

    // Guard against reflections: force det(R) = +1.
    const double d = (V * U.transpose()).determinant() > 0.0 ? 1.0 : -1.0;
    const Eigen::Matrix3d R = V * Eigen::Vector3d(1.0, 1.0, d).asDiagonal() * U.transpose();

    return {R, dstCentroid - R * srcCentroid};
}

EigenPair powerIteration(const Eigen::MatrixXd& A, int maxIter, double tol) {
    if (A.rows() != A.cols()) throw std::invalid_argument("powerIteration: matrix must be square");

    Eigen::VectorXd v = Eigen::VectorXd::Ones(A.rows()).normalized();
    double lambda = 0.0;
    for (int it = 1; it <= maxIter; ++it) {
        Eigen::VectorXd w = A * v;
        const double next = v.dot(w);  // Rayleigh quotient (v is unit length)
        v = w.normalized();
        if (std::abs(next - lambda) < tol * std::max(1.0, std::abs(next)))
            return {next, v, it};
        lambda = next;
    }
    return {lambda, v, maxIter};
}

Eigen::VectorXd stationaryDistribution(const Eigen::MatrixXd& P) {
    const Eigen::Index n = P.rows();
    if (n != P.cols()) throw std::invalid_argument("stationaryDistribution: matrix must be square");

    // (P^T - I) pi = 0 is rank-deficient; replace one equation with sum(pi) = 1.
    Eigen::MatrixXd A = P.transpose() - Eigen::MatrixXd::Identity(n, n);
    A.row(n - 1).setOnes();
    Eigen::VectorXd b = Eigen::VectorXd::Zero(n);
    b(n - 1) = 1.0;
    return A.fullPivLu().solve(b);
}

Eigen::VectorXd solvePoisson1D(const std::function<double(double)>& f, int n) {
    if (n < 1) throw std::invalid_argument("solvePoisson1D: n must be positive");
    const double h = 1.0 / (n + 1);

    // Tridiagonal matrix (1/h^2) * tridiag(-1, 2, -1), assembled from triplets.
    std::vector<Eigen::Triplet<double>> triplets;
    triplets.reserve(3 * static_cast<size_t>(n));
    Eigen::VectorXd rhs(n);
    for (int i = 0; i < n; ++i) {
        triplets.emplace_back(i, i, 2.0);
        if (i > 0) triplets.emplace_back(i, i - 1, -1.0);
        if (i + 1 < n) triplets.emplace_back(i, i + 1, -1.0);
        rhs(i) = h * h * f((i + 1) * h);
    }
    Eigen::SparseMatrix<double> K(n, n);
    K.setFromTriplets(triplets.begin(), triplets.end());

    // K is symmetric positive definite -> sparse Cholesky (LDL^T).
    Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> solver(K);
    if (solver.info() != Eigen::Success) throw std::runtime_error("solvePoisson1D: factorization failed");
    return solver.solve(rhs);
}

}  // namespace ef::linalg

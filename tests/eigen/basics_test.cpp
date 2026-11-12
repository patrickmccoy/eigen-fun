// "Learning tests" that document Eigen behaviour you might not expect.
#include <Eigen/Dense>
#include <gtest/gtest.h>
#include <vector>

using Eigen::MatrixXd;
using Eigen::VectorXd;

TEST(EigenBasics, CommaInitializerAndStorageOrderIsColumnMajor) {
    Eigen::Matrix<double, 2, 3> m;
    m << 1, 2, 3,
         4, 5, 6;
    EXPECT_EQ(m(1, 0), 4);
    // Default storage is column-major: memory is 1,4,2,5,3,6.
    EXPECT_EQ(m.data()[1], 4);
    EXPECT_EQ(m.data()[2], 2);
}

TEST(EigenBasics, BlockOperationsAreWritableViews) {
    MatrixXd m = MatrixXd::Zero(4, 4);
    m.block<2, 2>(1, 1).setConstant(7.0);  // views alias the original storage
    m.row(0).setLinSpaced(4, 0.0, 3.0);
    EXPECT_EQ(m(2, 2), 7.0);
    EXPECT_EQ(m(0, 3), 3.0);
    EXPECT_EQ(m.sum(), 4 * 7.0 + (0 + 1 + 2 + 3));
}

TEST(EigenBasics, ArrayVsMatrixSemantics) {
    Eigen::Matrix2d a;
    a << 1, 2, 3, 4;
    Eigen::Matrix2d matProduct = a * a;                         // linear algebra product
    Eigen::Matrix2d elemProduct = (a.array() * a.array()).matrix();  // coefficient-wise
    EXPECT_EQ(matProduct(0, 0), 1 * 1 + 2 * 3);
    EXPECT_EQ(elemProduct(0, 0), 1);
    EXPECT_EQ(elemProduct(1, 1), 16);
}

TEST(EigenBasics, BroadcastingAndReductions) {
    MatrixXd m(2, 3);
    m << 1, 2, 3,
         4, 5, 6;
    VectorXd colMeans = m.colwise().mean().transpose();
    EXPECT_TRUE(colMeans.isApprox(Eigen::Vector3d(2.5, 3.5, 4.5)));

    MatrixXd centered = m.rowwise() - m.colwise().mean();
    EXPECT_NEAR(centered.colwise().sum().norm(), 0.0, 1e-12);

    Eigen::Index r, c;
    m.maxCoeff(&r, &c);
    EXPECT_EQ(r, 1);
    EXPECT_EQ(c, 2);
}

TEST(EigenBasics, MapWrapsExistingMemoryWithoutCopying) {
    std::vector<double> raw = {1, 2, 3, 4, 5, 6};
    Eigen::Map<Eigen::Matrix<double, 2, 3>> view(raw.data());
    view(0, 0) = 100;  // writes straight into the std::vector
    EXPECT_EQ(raw[0], 100);
    EXPECT_EQ(view(1, 2), 6);
}

TEST(EigenBasics, AliasingWithTransposeRequiresCare) {
    Eigen::Matrix3d m;
    m << 1, 2, 3, 4, 5, 6, 7, 8, 9;
    const Eigen::Matrix3d expected = m.transpose();
    // `m = m.transpose()` is a classic aliasing bug (Eigen asserts on it in debug).
    // The in-place variant is safe:
    m.transposeInPlace();
    EXPECT_EQ(m, expected);

    // Matrix products assume aliasing and evaluate into a temporary; noalias()
    // opts out of that when you know the destination is distinct.
    Eigen::Matrix3d viaTemporary = m * expected;  // safe default
    Eigen::Matrix3d out;
    out.noalias() = m * expected;               // skips the temporary
    EXPECT_TRUE(out.isApprox(viaTemporary));

    // But noalias() on a genuinely aliased product gives garbage:
    Eigen::Matrix3d aliased = m;
    aliased.noalias() = aliased * expected;  // reads entries it already overwrote
    EXPECT_FALSE(aliased.isApprox(viaTemporary));
}

TEST(EigenBasics, ExpressionTemplatesAreLazy) {
    VectorXd a = VectorXd::Ones(3), b = VectorXd::Ones(3);
    auto expr = a + b;   // `auto` captures an expression, NOT a vector...
    a.setZero();         // ...so mutating `a` changes what `expr` evaluates to.
    VectorXd evaluated = expr;
    EXPECT_EQ(evaluated, VectorXd::Ones(3));
}

TEST(EigenBasics, SolvingLinearSystemsWithDifferentDecompositions) {
    MatrixXd A = MatrixXd::Random(50, 50);
    A = A * A.transpose() + 50 * MatrixXd::Identity(50, 50);  // make SPD
    VectorXd xTrue = VectorXd::LinSpaced(50, -1.0, 1.0);
    VectorXd b = A * xTrue;

    EXPECT_TRUE(A.partialPivLu().solve(b).isApprox(xTrue, 1e-10));
    EXPECT_TRUE(A.llt().solve(b).isApprox(xTrue, 1e-10));
    EXPECT_TRUE(A.householderQr().solve(b).isApprox(xTrue, 1e-10));
}

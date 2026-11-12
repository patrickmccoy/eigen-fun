// How move semantics interact with Eigen's dynamic vs fixed-size types.
#include <Eigen/Dense>
#include <gtest/gtest.h>
#include <utility>
#include <vector>

TEST(EigenMove, DynamicMatrixMoveStealsHeapBuffer) {
    Eigen::MatrixXd a = Eigen::MatrixXd::Random(500, 500);
    const double* storage = a.data();
    Eigen::MatrixXd b = std::move(a);
    EXPECT_EQ(b.data(), storage);
    EXPECT_EQ(a.size(), 0);  // Eigen leaves a moved-from dynamic matrix empty
}

TEST(EigenMove, FixedSizeMatrixMoveIsACopy) {
    // Fixed-size objects live inline (often on the stack) -- there's no pointer
    // to steal, so "moving" copies the 16 doubles. Moves only help heap owners.
    Eigen::Matrix4d a = Eigen::Matrix4d::Random();
    const Eigen::Matrix4d original = a;
    Eigen::Matrix4d b = std::move(a);
    EXPECT_NE(b.data(), a.data());
    EXPECT_EQ(b, original);
    EXPECT_EQ(a, original);  // source unchanged
}

TEST(EigenMove, SwapIsConstantTimeForDynamicMatrices) {
    Eigen::MatrixXd a = Eigen::MatrixXd::Ones(1000, 1000);
    Eigen::MatrixXd b = Eigen::MatrixXd::Zero(10, 10);
    const double* pa = a.data();
    const double* pb = b.data();
    a.swap(b);
    EXPECT_EQ(a.data(), pb);
    EXPECT_EQ(b.data(), pa);
    EXPECT_EQ(b.rows(), 1000);
}

TEST(EigenMove, ReturningAMatrixFromAFunctionDoesNotCopy) {
    const double* inner = nullptr;
    auto build = [&inner] {
        Eigen::MatrixXd m(200, 200);
        m.setRandom();
        inner = m.data();
        return m;  // NRVO or move -- either way, no deep copy
    };
    Eigen::MatrixXd result = build();
    EXPECT_EQ(result.data(), inner);
}

TEST(EigenMove, VectorOfDynamicMatricesReallocatesByMoving) {
    std::vector<Eigen::MatrixXd> v;
    v.emplace_back(Eigen::MatrixXd::Random(100, 100));
    const double* storage = v[0].data();
    for (int i = 0; i < 64; ++i) v.emplace_back(10, 10);  // force several reallocations
    EXPECT_EQ(v[0].data(), storage);
}

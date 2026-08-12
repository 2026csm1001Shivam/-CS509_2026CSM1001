#include "gemm.h"

#include <algorithm>
#include <cmath>
#include <iostream>

void matmul_simple(const Matrix &A, const Matrix &B, Matrix &C) {
    const int M = A.rows();
    const int K = A.cols();
    const int N = B.cols();
    C.zero();

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double sum = 0.0;
            for (int k = 0; k < K; ++k) {
                sum += A.at(i, k) * B.at(k, j);
            }
            C.at(i, j) = sum;
        }
    }
}

void matmul_blocked(const Matrix &A, const Matrix &B, Matrix &C, int bs) {
    const int M = A.rows();
    const int K = A.cols();
    const int N = B.cols();
    C.zero();

    if (bs < 1) bs = 1;

    for (int ii = 0; ii < M; ii += bs) {
        const int i_max = std::min(ii + bs, M);

        for (int kk = 0; kk < K; kk += bs) {
            const int k_max = std::min(kk + bs, K);

            for (int jj = 0; jj < N; jj += bs) {
                const int j_max = std::min(jj + bs, N);

                for (int i = ii; i < i_max; ++i) {
                    for (int k = kk; k < k_max; ++k) {
                        const double a_ik = A.at(i, k);
                        for (int j = jj; j < j_max; ++j) {
                            C.at(i, j) += a_ik * B.at(k, j);
                        }
                    }
                }
            }
        }
    }
}

bool matrices_equal(const Matrix &X, const Matrix &Y, double tol) {
    if (X.rows() != Y.rows() || X.cols() != Y.cols()) return false;

    for (int i = 0; i < X.rows(); ++i) {
        for (int j = 0; j < X.cols(); ++j) {
            if (std::fabs(X.at(i, j) - Y.at(i, j)) > tol) {
                std::cout << "Mismatch at (" << i << "," << j << "): "
                          << X.at(i, j) << " vs " << Y.at(i, j) << "\n";
                return false;
            }
        }
    }
    return true;
}

double matrix_checksum(const Matrix &M) {
    double s = 0.0;
    for (int i = 0; i < M.rows(); ++i) {
        for (int j = 0; j < M.cols(); ++j) {
            s += M.at(i, j);
        }
    }
    return s;
}

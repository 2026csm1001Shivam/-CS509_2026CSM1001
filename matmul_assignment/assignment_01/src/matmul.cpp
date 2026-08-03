/*
 * ============================================================
 *  Assignment 01 - Matrix Multiplication: Simple vs Blocked
 *  Core algorithm library (implementation)
 *  Course: CS509 - Lab Work, First-Year M.Tech CSE (2026)
 * ============================================================
 *
 *  Contains only the core algorithms (no I/O, no timing).
 *  See matmul.h for documentation of each function.
 * ============================================================
 */

#include "matmul.h"

#include <iostream>
#include <iomanip>
#include <random>
#include <cmath>
#include <algorithm>

/* ----------------------- Matrix ---------------------------- */

Matrix::Matrix(int n) : n_(n), data_(static_cast<std::size_t>(n) * n, 0.0) {}

void Matrix::fill_random(unsigned int seed) {
    std::mt19937 gen(seed);
    std::uniform_int_distribution<int> dist(0, 9); // small ints, easy to eyeball
    for (auto &v : data_) v = static_cast<double>(dist(gen));
}

void Matrix::zero() { std::fill(data_.begin(), data_.end(), 0.0); }

void Matrix::print() const {
    for (int i = 0; i < n_; ++i) {
        for (int j = 0; j < n_; ++j) {
            std::cout << std::setw(6) << std::fixed << std::setprecision(1) << at(i, j) << " ";
        }
        std::cout << "\n";
    }
}

/* ------------------- Simple multiplication ----------------- */

void matmul_simple(const Matrix &A, const Matrix &B, Matrix &C) {
    int n = A.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            double sum = 0.0;
            for (int k = 0; k < n; ++k) {
                sum += A.at(i, k) * B.at(k, j);
            }
            C.at(i, j) = sum;
        }
    }
}

/* ------------------- Blocked multiplication ---------------- */

void matmul_blocked(const Matrix &A, const Matrix &B, Matrix &C, int bs) {
    int n = A.size();
    C.zero();

    for (int ii = 0; ii < n; ii += bs) {
        int i_max = std::min(ii + bs, n);

        for (int jj = 0; jj < n; jj += bs) {
            int j_max = std::min(jj + bs, n);

            for (int kk = 0; kk < n; kk += bs) {
                int k_max = std::min(kk + bs, n);

                /* Multiply the current block */
                for (int i = ii; i < i_max; ++i) {
                    for (int j = jj; j < j_max; ++j) {
                        double sum = C.at(i, j);
                        for (int k = kk; k < k_max; ++k) {
                            sum += A.at(i, k) * B.at(k, j);
                        }
                        C.at(i, j) = sum;
                    }
                }
            }
        }
    }
}

/* -------------------- Verification helpers ----------------- */

bool matrices_equal(const Matrix &X, const Matrix &Y, double tol) {
    int n = X.size();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
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
    int n = M.size();
    double s = 0.0;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            s += M.at(i, j);
        }
    }
    return s;
}

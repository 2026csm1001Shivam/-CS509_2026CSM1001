/*
 * ============================================================
 *  Assignment 01 - Matrix Multiplication: Simple vs Blocked
 *  Core algorithm library (header)
 *  Course: CS509 - Lab Work, First-Year M.Tech CSE (2026)
 * ============================================================
 *
 *  This header declares the core matrix-multiplication
 *  algorithms used by Assignment 01:
 *
 *    1. matmul_simple  -> naive i-j-k triple loop       O(n^3)
 *    2. matmul_blocked -> cache-friendly tiled version   O(n^3)
 *
 *  Both compute C = A x B for N x N matrices of doubles.
 *  The algorithms are kept separate from the driver
 *  (assignment_01/driver/driver.cpp) so that the driver can
 *  be replaced or reused without touching the core code.
 * ============================================================
 */

#ifndef MATMUL_H
#define MATMUL_H

#include <vector>
#include <cstddef>

/* A simple wrapper around a flat std::vector<double> so we can
 * index it as a 2D N x N matrix while keeping the underlying
 * storage contiguous (important for cache behaviour). */
class Matrix {
public:
    explicit Matrix(int n);

    inline double &at(int i, int j) { return data_[static_cast<std::size_t>(i) * n_ + j]; }
    inline double at(int i, int j) const { return data_[static_cast<std::size_t>(i) * n_ + j]; }

    int size() const { return n_; }
    double *raw() { return data_.data(); }
    const double *raw() const { return data_.data(); }

    /* Fills the matrix with reproducible pseudo-random values
     * in [0, 9] using the given seed (input preparation - this
     * is NOT part of the timed algorithm region). */
    void fill_random(unsigned int seed);

    void zero();
    void print() const;

private:
    int n_;
    std::vector<double> data_;
};

/* ------------------------------------------------------------
 * 1) SIMPLE (NAIVE) MATRIX MULTIPLICATION
 *    C[i][j] = sum_k A[i][k] * B[k][j]
 *    Loop order: i -> j -> k
 *    Simple to read but NOT cache friendly: B is accessed
 *    column-wise (stride n) in the inner loop.
 * ---------------------------------------------------------- */
void matmul_simple(const Matrix &A, const Matrix &B, Matrix &C);

/* ------------------------------------------------------------
 * 2) BLOCKED / TILED MATRIX MULTIPLICATION
 *    The N x N matrices are divided into BS x BS blocks; each
 *    block's data stays resident in cache while it is reused.
 * ---------------------------------------------------------- */
void matmul_blocked(const Matrix &A, const Matrix &B, Matrix &C, int bs);

/* Compare two matrices element-wise within a tolerance. */
bool matrices_equal(const Matrix &X, const Matrix &Y, double tol);

/* Sum of all elements - a compact scalar "fingerprint" of the
 * result matrix used to represent the computed result. */
double matrix_checksum(const Matrix &M);

#endif /* MATMUL_H */

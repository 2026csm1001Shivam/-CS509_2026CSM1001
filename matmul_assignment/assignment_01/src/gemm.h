#ifndef GEMM_H
#define GEMM_H

#include "matrix.h"

void matmul_simple(const Matrix &A, const Matrix &B, Matrix &C);

void matmul_blocked(const Matrix &A, const Matrix &B, Matrix &C, int bs);

bool matrices_equal(const Matrix &X, const Matrix &Y, double tol);

double matrix_checksum(const Matrix &M);

#endif

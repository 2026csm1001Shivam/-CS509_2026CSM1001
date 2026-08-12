#ifndef MATRIX_H
#define MATRIX_H

#include <vector>
#include <cstddef>

class Matrix {
public:
    Matrix(int rows, int cols);

    inline double &at(int i, int j) { return data_[static_cast<std::size_t>(i) * cols_ + j]; }
    inline double at(int i, int j) const { return data_[static_cast<std::size_t>(i) * cols_ + j]; }

    int rows() const { return rows_; }
    int cols() const { return cols_; }

    double *raw() { return data_.data(); }
    const double *raw() const { return data_.data(); }

    void zero();
    void print() const;

private:
    int rows_;
    int cols_;
    std::vector<double> data_;
};

#endif

#include "matrix.h"

#include <iostream>
#include <algorithm>

Matrix::Matrix(int rows, int cols)
    : rows_(rows),
      cols_(cols),
      data_(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols), 0.0) {}

void Matrix::zero() { std::fill(data_.begin(), data_.end(), 0.0); }

void Matrix::print() const {
    for (int i = 0; i < rows_; ++i) {
        for (int j = 0; j < cols_; ++j) {
            std::cout << at(i, j) << " ";
        }
        std::cout << "\n";
    }
}

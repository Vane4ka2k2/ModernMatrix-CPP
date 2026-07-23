#include <iostream>
#include "matrix.hpp"

// Пример использования класса math::Matrix.
// Показывает создание единичной, заполненной и перемножение матриц.
int main()
{
    using math::Matrix;
    Matrix<double> a = Matrix<double>::identity(3); // Единичная матрица 3x3.
    Matrix<double> b = Matrix<double>::fill(3, 3, 2.0); // Матрица 3x3, заполненная двойками.
    Matrix<double> c = a * b; // Результат матричного умножения.

    std::cout << "Matrix A:\n" << a << "\n\n";
    std::cout << "Matrix B:\n" << b << "\n\n";
    std::cout << "Matrix C = A * B:\n" << c << "\n";
    return 0;
}

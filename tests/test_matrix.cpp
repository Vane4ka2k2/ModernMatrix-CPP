#include "matrix.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

using math::Matrix;

// Простая проверка условия, прерывающая тесты при сбое.
static void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::abort();
    }
}

int main() {
    {
        // Проверка создания динамической матрицы по умолчанию.
        Matrix<int> empty;
        expect(empty.rows() == 0 && empty.cols() == 0, "Dynamic default matrix should be 0x0");
    }

    {
        // Проверка конструктора списка инициализации.
        Matrix<double> a{{1.0, 2.0}, {3.0, 4.0}};
        expect(a.rows() == 2 && a.cols() == 2, "Initializer list size mismatch");
        expect(a(0, 1) == 2.0, "Initializer list value mismatch");
    }

    {
        // Проверка арифметики для статических матриц 2x2.
        Matrix<double, 2, 2> a{{1.0, 2.0}, {3.0, 4.0}};
        Matrix<double, 2, 2> b{{5.0, 6.0}, {7.0, 8.0}};
        auto sum = a + b;
        expect(sum(0, 0) == 6.0 && sum(1, 1) == 12.0, "Matrix addition failed");
        auto diff = b - a;
        expect(diff(0, 0) == 4.0 && diff(1, 1) == 4.0, "Matrix subtraction failed");
        auto prod = a * b;
        expect(prod(0, 0) == 19.0 && prod(1, 1) == 50.0, "Matrix multiplication failed");
    }

    {
        // Проверка транспонирования динамической матрицы.
        Matrix<double> mat(2, 3);
        mat(0, 0) = 1.0;
        mat(0, 1) = 2.0;
        mat(0, 2) = 3.0;
        mat(1, 0) = 4.0;
        mat(1, 1) = 5.0;
        mat(1, 2) = 6.0;
        auto trans = mat.transpose();
        expect(trans.rows() == 3 && trans.cols() == 2, "Transpose dimensions incorrect");
        expect(trans(2, 1) == 6.0, "Transpose values incorrect");
    }

    {
        // Проверка вычисления детерминанта и обратной матрицы для 2x2.
        Matrix<double> m{{4.0, 7.0}, {2.0, 6.0}};
        expect(m.determinant() == 10.0, "Determinant computation failed");
        auto inv = m.inverse();
        expect(inv.rows() == 2 && inv.cols() == 2, "Inverse dimensions failed");
        expect(std::abs(inv(0, 0) - 0.6) < 1e-9, "Inverse value incorrect");
        expect(std::abs(inv(0, 1) + 0.7) < 1e-9, "Inverse value incorrect");
    }

    {
        // Проверка выбрасывания исключения при сложении матриц разного размера.
        Matrix<double> a{{1.0, 2.0}, {3.0, 4.0}};
        Matrix<double> b{{1.0, 2.0, 3.0}};
        bool caught = false;
        try {
            auto c = a + b;
            (void)c;
        } catch (const std::invalid_argument&) {
            caught = true;
        }
        expect(caught, "Mismatched shape addition should throw invalid_argument");
    }

    {
        // Проверка текстового ввода/вывода матрицы через поток.
        std::stringstream ss;
        Matrix<int> m{{1, 2}, {3, 4}};
        ss << m;
        Matrix<int> loaded(2, 2);
        ss.seekg(0);
        ss >> loaded;
        expect(loaded == m, "Stream I/O roundtrip failed");
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}

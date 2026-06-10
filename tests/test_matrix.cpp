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

    {
        // Проверка безопасного доступа через at() с проверкой границ.
        Matrix<double> m{{1.0, 2.0}, {3.0, 4.0}};
        expect(m.at(0, 0) == 1.0, "at() access failed");
        expect(m.at(1, 1) == 4.0, "at() access failed");
        bool caught = false;
        try {
            m.at(2, 0);
        } catch (const std::out_of_range&) {
            caught = true;
        }
        expect(caught, "at() should throw out_of_range for invalid index");
    }

    {
        // Проверка доступа к столбцам через col().
        Matrix<double> m(3, 3);
        m(0, 0) = 1.0; m(0, 1) = 2.0; m(0, 2) = 3.0;
        m(1, 0) = 4.0; m(1, 1) = 5.0; m(1, 2) = 6.0;
        m(2, 0) = 7.0; m(2, 1) = 8.0; m(2, 2) = 9.0;
        
        auto col0 = m.col(0);
        expect(col0[0] == 1.0 && col0[1] == 4.0 && col0[2] == 7.0, "col() access failed");
        
        auto col1 = m.col(1);
        expect(col1[0] == 2.0 && col1[1] == 5.0 && col1[2] == 8.0, "col() access failed");
    }

    {
        // Проверка динамического изменения размера матрицы.
        Matrix<double> m(2, 2);
        m.resize(3, 3);
        expect(m.rows() == 3 && m.cols() == 3, "resize() size change failed");
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}

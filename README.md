<div align="center">

# 🧮 ModernMatrix-CPP — Библиотека линейной алгебры на C++23

**Высокопроизводительная шаблонная библиотека для работы с матрицами и выполнения операций линейной алгебры на современном C++23.**

![C++23](https://img.shields.io/badge/C%2B%2B-23-00599C?logo=cplusplus)
![Header Only](https://img.shields.io/badge/Library-Header--Only-orange)
![License](https://img.shields.io/badge/License-MIT-green)

</div>

---

## 📑 Оглавление
- [🌟 Основные возможности](#-основные-возможности)
- [🛠 Требования и Сборка](#-требования-и-сборка)
- [💡 Пример кода](#-пример-кода)
- [📜 Лицензия](#-лицензия)

---

## 🌟 Основные возможности

- 📐 **Поддержка динамических и статических матриц** — Оптимизированный класс `Matrix<T>`.
- ➕ **Арифметика матриц** — Сложение, вычитание, умножение матриц и скаляров.
- 🔄 **Алгоритмы линейной алгебры** — Транспонирование, поиск определителя (детерминанта), обращение матриц.
- ⚡ **Современные фичи C++23** — Использование концептов (`concepts`), `constexpr` и шаблонов.

---

## 🛠 Требования и Сборка

### Требования:
- Компилятор с поддержкой C++23 (GCC 13+, Clang 16+ или MSVC 2022)
- CMake 3.20+

### Запуск тестов:
```bash
git clone https://github.com/Vane4ka2k2/ModernMatrix-CPP.git
cd ModernMatrix-CPP
cmake -B build
cmake --build build
```

---

## 💡 Пример кода

```cpp
#include "matrix.hpp"
#include <iostream>

int main() {
    Matrix<double> A = {{1, 2}, {3, 4}};
    Matrix<double> B = {{5, 6}, {7, 8}};
    
    auto C = A * B;
    std::cout << "Determinant of A: " << A.determinant() << "\n";
    return 0;
}
```

---

## 📜 Лицензия

Распространяется под лицензией **MIT**. Подробнее см. в файле [LICENSE](LICENSE).
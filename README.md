# NewMatrix

Проект `NewMatrix` реализует шаблонный класс `math::Matrix<T, Rows, Cols, Allocator>` для линейной алгебры на C++23.

## Описание

`math::Matrix` поддерживает:
- статические и динамические матрицы
- арифметические операции и матричное умножение
- транспонирование, след, детерминант и обратную матрицу
- сравнение и ввод/вывод в текстовом и бинарном форматах

## Быстрая сборка и запуск

```bash
./build_and_run.sh
```

## Сборка и запуск тестов

```bash
./build_and_run_tests.sh
```

## Ручная сборка

```bash
mkdir -p build
cd build
cmake ..
cmake --build . --target NewMatrix
cmake --build . --target matrix_tests
ctest --output-on-failure
```

## Пример использования

```cpp
#include <iostream>
#include "matrix.hpp"

int main() {
    using math::Matrix;

    Matrix<double> a = Matrix<double>::identity(3);
    Matrix<double> b = Matrix<double>::fill(3, 3, 2.0);
    Matrix<double> c = a * b;

    std::cout << "Matrix A:\n" << a << "\n\n";
    std::cout << "Matrix B:\n" << b << "\n\n";
    std::cout << "Matrix C = A * B:\n" << c << "\n";

    return 0;
}
```

## Структура проекта

- `main.cpp` — пример использования `Matrix`
- `matrix.hpp` — реализация шаблонного класса `math::Matrix`
- `tests/test_matrix.cpp` — модульные тесты
- `CMakeLists.txt` — настройка сборки
- `build_and_run.sh` — скрипт для сборки и запуска приложения
- `build_and_run_tests.sh` — скрипт для сборки и запуска тестов
- `docs/MatrixAPI.md` — подробное описание API

## Документация

Полная API документация доступна в `docs/MatrixAPI.md`.

## Требования

- CMake 3.23+
- Компилятор C++23

#ifndef MATH_MATRIX_HPP
#define MATH_MATRIX_HPP

// Заголовочный файл реализует шаблонный класс math::Matrix для линейной алгебры.
// Поддерживается статическая и динамическая размерность, основные операции,
// доступ по элементам, преобразования, сравнение и простой ввод/вывод.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <compare>
#include <concepts>
#include <exception>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <memory_resource>
#include <numeric>
#include <stdexcept>
#include <span>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>
#include <ranges>

#ifdef __cpp_lib_format
#include <format>
#endif

namespace math {

// Константа для обозначения динамической размерности матрицы.
// Если Rows или Cols равны Dynamic, размеры задаются во время выполнения.
static constexpr std::size_t Dynamic = static_cast<std::size_t>(-1);

// Концепт, разрешающий только арифметические типы для элементов матрицы.
template<typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

// Вспомогательные утилиты и концепты, которые не предназначены для прямого использования.
namespace detail {
    template<typename T>
    [[nodiscard]] constexpr T abs_value(T value) noexcept {
        if constexpr (std::is_unsigned_v<T>) {
            return value;
        } else {
            return value < T{} ? -value : value;
        }
    }

    template<typename T>
    constexpr T default_epsilon() noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(1e-6);
        } else {
            return T{};
        }
    }

    template<typename T>
    concept ThreeWayComparable = requires(const T& a, const T& b) {
        { a <=> b } -> std::convertible_to<std::strong_ordering>;
    };
}

// Концепт выражения матрицы. Позволяет принимать в качестве аргументов
// как сами матрицы, так и выражения на их основе.
template<typename E>
concept MatrixExpression = requires(const E& expr, std::size_t i, std::size_t j) {
    { expr.rows() } -> std::convertible_to<std::size_t>;
    { expr.cols() } -> std::convertible_to<std::size_t>;
    { expr(i, j) };
    typename E::value_type;
};

// Основной класс матрицы с поддержкой статической и динамической размерности.
// Шаблонные параметры:
// T — тип элемента,
// Rows, Cols — размеры (или Dynamic для динамических размеров),
// Allocator — аллокатор для динамической матрицы.
template<Arithmetic T,
         std::size_t Rows = Dynamic,
         std::size_t Cols = Dynamic,
         typename Allocator = std::pmr::polymorphic_allocator<T>>
class Matrix {
public:
    using value_type = T;
    using size_type = std::size_t;
    using allocator_type = Allocator;

    static constexpr size_type rows_static = Rows;
    static constexpr size_type cols_static = Cols;
    static constexpr bool has_static_rows = Rows != Dynamic;
    static constexpr bool has_static_cols = Cols != Dynamic;
    static constexpr bool is_static = has_static_rows && has_static_cols;
    static constexpr T epsilon = detail::default_epsilon<T>();

    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /**
     * \brief Конструктор по умолчанию.
     * 
     * Для статических матриц сохраняет фиксированные размеры и инициализирует элементы нулем.
     * Для динамических матриц создает матрицу размером 0x0.
     */
    Matrix() noexcept {
        if constexpr (has_static_rows) {
            rows_ = Rows;
        }
        if constexpr (has_static_cols) {
            cols_ = Cols;
        }
        if constexpr (is_static) {
            data_.fill(T{});
        }
    }

    /**
     * \brief Конструктор для создания динамической матрицы.
     * \param rows Количество строк.
     * \param cols Количество столбцов.
     * \note Только для динамических матриц.
     * \warning Выделяет память и бросает исключение если рассчиты превышают память.
     */
    Matrix(size_type rows, size_type cols) requires (!is_static) {
        set_size(rows, cols);
        init_storage();
    }

    /**
     * \brief Конструктор из вложенного списка инициализации.
     * \param init Нестед initializer list для строк и столбцов.
     * \throw std::invalid_argument Если размеры не совпадают с статическими.
     */
    Matrix(std::initializer_list<std::initializer_list<T>> init) {
        const size_type rows = init.size();
        const size_type cols = rows ? std::begin(init)->size() : 0;

        for (auto const& row : init) {
            if (row.size() != cols) {
                throw std::invalid_argument("Matrix initializer list has inconsistent row sizes");
            }
        }

        if constexpr (has_static_rows) {
            if (rows != Rows) {
                throw std::invalid_argument("Matrix initializer list size does not match static row count");
            }
        }
        if constexpr (has_static_cols) {
            if (cols != Cols) {
                throw std::invalid_argument("Matrix initializer list size does not match static column count");
            }
        }

        if constexpr (!is_static) {
            set_size(rows, cols);
            data_.assign(rows * cols, T{});
        }

        auto out = data();
        size_type index = 0;
        for (auto const& row : init) {
            for (auto const& value : row) {
                out[index++] = value;
            }
        }

        if constexpr (is_static) {
            if (index < data_.size()) {
                std::fill(data_.begin() + index, data_.end(), T{});
            }
        }
    }

    Matrix(const Matrix&) = default;
    Matrix(Matrix&&) noexcept = default;
    Matrix& operator=(const Matrix&) = default;
    Matrix& operator=(Matrix&&) noexcept = default;

    template<Arithmetic U, std::size_t R2, std::size_t C2, typename Alloc2>
    requires std::convertible_to<U, T>
    Matrix(const Matrix<U, R2, C2, Alloc2>& other) {
        assign_from(other);
    }

    template<MatrixExpression E>
    requires std::convertible_to<typename E::value_type, T>
    Matrix(const E& expression) {
        assign_expression(expression);
    }

    Matrix& operator=(MatrixExpression auto const& expression) {
        assign_expression(expression);
        return *this;
    }

    constexpr size_type rows() const noexcept {
        if constexpr (has_static_rows) {
            return Rows;
        }
        return rows_;
    }

    constexpr size_type cols() const noexcept {
        if constexpr (has_static_cols) {
            return Cols;
        }
        return cols_;
    }

    constexpr size_type size() const noexcept {
        return rows() * cols();
    }

    constexpr T* data() noexcept {
        if constexpr (is_static) {
            return data_.data();
        }
        return data_.data();
    }

    constexpr const T* data() const noexcept {
        if constexpr (is_static) {
            return data_.data();
        }
        return data_.data();
    }

    // Прямой доступ к данным для динамических матриц.
    // Используется только тогда, когда матрица не является полностью статической.
    T* data() noexcept requires (!is_static) {
        return data_.data();
    }

    const T* data() const noexcept requires (!is_static) {
        return data_.data();
    }

    // Возвращает span для строки матрицы. Удобно для итерации по строке.
    constexpr std::span<T> row(size_type i) noexcept {
        assert(i < rows());
        return {data() + i * cols(), cols()};
    }

    constexpr std::span<const T> row(size_type i) const noexcept {
        assert(i < rows());
        return {data() + i * cols(), cols()};
    }

    /**
     * Вспомогательный класс для доступа к столбцу матрицы.
     * Позволяет эффективно работать со столбцами несмежно хранящихся данных.
     */
    class ColumnProxy {
    private:
        T* data_;
        size_type cols_;
        size_type size_;
    public:
        ColumnProxy(T* data, size_type cols, size_type size) 
            : data_(data), cols_(cols), size_(size) {}
        T& operator[](size_type i) noexcept {
            assert(i < size_);
            return data_[i * cols_];
        }
        const T& operator[](size_type i) const noexcept {
            assert(i < size_);
            return data_[i * cols_];
        }
    };

    /**
     * Возвращает прокси-объект столбца для доступа через оператор [].
     * Использование: T& val = matrix.col(j)[i];
     * Примечание: столбцы менее эффективны для row-major layout.
     */
    ColumnProxy col(size_type j) noexcept {
        assert(j < cols());
        return ColumnProxy(data() + j, cols(), rows());
    }

    /**
     * Константная версия метода col() для доступа к элементам столбца.
     */
    const ColumnProxy col(size_type j) const noexcept {
        assert(j < cols());
        return ColumnProxy(data() + j, cols(), rows());
    }

    // Прокси-строка для доступа через оператор[]: matrix[i][j].
    struct RowProxy {
        T* row;
        size_type length;
        T& operator[](size_type j) const {
            assert(j < length);
            return row[j];
        }
    };

    struct ConstRowProxy {
        const T* row;
        size_type length;
        const T& operator[](size_type j) const {
            assert(j < length);
            return row[j];
        }
    };

    RowProxy operator[](size_type i) {
        assert(i < rows());
        return {data() + i * cols(), cols()};
    }

    ConstRowProxy operator[](size_type i) const {
        assert(i < rows());
        return {data() + i * cols(), cols()};
    }

    // Доступ к элементам по индексу (без проверки границ в релизе,
    // с assert в отладочной сборке).
    T& operator()(size_type i, size_type j) {
        assert(i < rows() && j < cols());
        return data()[i * cols() + j];
    }

    const T& operator()(size_type i, size_type j) const {
        assert(i < rows() && j < cols());
        return data()[i * cols() + j];
    }

    /**
     * Безопасный доступ к элементу с проверкой границ.
     * Бросает std::out_of_range при выходе за границы.
     */
    T& at(size_type i, size_type j) {
        if (i >= rows() || j >= cols()) {
            throw std::out_of_range("Matrix index out of range");
        }
        return data()[i * cols() + j];
    }

    /**
     * Константная версия at() для безопасного доступа к элементу.
     */
    const T& at(size_type i, size_type j) const {
        if (i >= rows() || j >= cols()) {
            throw std::out_of_range("Matrix index out of range");
        }
        return data()[i * cols() + j];
    }

    constexpr iterator begin() noexcept {
        return data();
    }

    constexpr iterator end() noexcept {
        return data() + size();
    }

    constexpr const_iterator begin() const noexcept {
        return data();
    }

    constexpr const_iterator end() const noexcept {
        return data() + size();
    }

    constexpr const_iterator cbegin() const noexcept {
        return data();
    }

    constexpr const_iterator cend() const noexcept {
        return data() + size();
    }

    // Методы итераторов для совместимости со STL и диапазонами.
    constexpr reverse_iterator rbegin() noexcept {
        return reverse_iterator(end());
    }

    constexpr reverse_iterator rend() noexcept {
        return reverse_iterator(begin());
    }

    constexpr const_reverse_iterator rbegin() const noexcept {
        return const_reverse_iterator(end());
    }

    constexpr const_reverse_iterator rend() const noexcept {
        return const_reverse_iterator(begin());
    }

    // Представление строк в виде диапазона std::views.
    // Удобно для использования с алгоритмами C++20.
    auto rows_view() noexcept {
        return std::views::iota(size_type{0}, rows())
            | std::views::transform([this](size_type i) { return row(i); });
    }

    auto rows_view() const noexcept {
        return std::views::iota(size_type{0}, rows())
            | std::views::transform([this](size_type i) { return row(i); });
    }

    // Поэлементные арифметические операции.
    Matrix& operator+=(const Matrix& other) {
        ensure_same_shape(other);
        for (size_type i = 0; i < size(); ++i) {
            data()[i] += other.data()[i];
        }
        return *this;
    }

    Matrix& operator-=(const Matrix& other) {
        ensure_same_shape(other);
        for (size_type i = 0; i < size(); ++i) {
            data()[i] -= other.data()[i];
        }
        return *this;
    }

    Matrix& operator*=(const T& scalar) {
        for (size_type i = 0; i < size(); ++i) {
            data()[i] *= scalar;
        }
        return *this;
    }

    Matrix& operator/=(const T& scalar) {
        if (scalar == T{}) {
            throw std::invalid_argument("Division by zero in Matrix scalar division");
        }
        for (size_type i = 0; i < size(); ++i) {
            data()[i] /= scalar;
        }
        return *this;
    }

    template<MatrixExpression E>
    requires std::convertible_to<typename E::value_type, T>
    Matrix& operator+=(const E& expression) {
        ensure_same_shape(expression);
        for (size_type i = 0; i < rows(); ++i) {
            for (size_type j = 0; j < cols(); ++j) {
                (*this)(i, j) += static_cast<T>(expression(i, j));
            }
        }
        return *this;
    }

    template<MatrixExpression E>
    requires std::convertible_to<typename E::value_type, T>
    Matrix& operator-=(const E& expression) {
        ensure_same_shape(expression);
        for (size_type i = 0; i < rows(); ++i) {
            for (size_type j = 0; j < cols(); ++j) {
                (*this)(i, j) -= static_cast<T>(expression(i, j));
            }
        }
        return *this;
    }

    friend Matrix operator+(Matrix lhs, const Matrix& rhs) {
        lhs += rhs;
        return lhs;
    }

    friend Matrix operator-(Matrix lhs, const Matrix& rhs) {
        lhs -= rhs;
        return lhs;
    }

    friend Matrix operator*(Matrix lhs, const T& scalar) {
        lhs *= scalar;
        return lhs;
    }

    friend Matrix operator*(const T& scalar, Matrix rhs) {
        rhs *= scalar;
        return rhs;
    }

    friend Matrix operator/(Matrix lhs, const T& scalar) {
        lhs /= scalar;
        return lhs;
    }

    // Матричное умножение. Возвращает новую матрицу с размерностью rows x other.cols().
    // Для больших матриц используется блочная реализация для лучшей кэш-локальности.
    template<Arithmetic U, std::size_t R2, std::size_t C2, typename Alloc2>
    auto operator*(const Matrix<U, R2, C2, Alloc2>& other) const {
        using result_type = std::common_type_t<T, U>;
        constexpr size_type out_rows = Rows;
        constexpr size_type out_cols = C2;
        using result_matrix = Matrix<result_type, out_rows, out_cols, Allocator>;

        if (cols() != other.rows()) {
            throw std::invalid_argument("Matrix multiplication dimension mismatch");
        }

        result_matrix result;
        if constexpr (!result_matrix::is_static) {
            result.resize(rows(), other.cols());
        }

        const size_type r = rows();
        const size_type common = cols();
        const size_type c = other.cols();

        if (r > 64 && common > 64 && c > 64) {
            const size_type block = 32;
            for (size_type ii = 0; ii < r; ii += block) {
                for (size_type kk = 0; kk < common; kk += block) {
                    for (size_type jj = 0; jj < c; jj += block) {
                        const size_type iend = std::min(ii + block, r);
                        const size_type kend = std::min(kk + block, common);
                        const size_type jend = std::min(jj + block, c);
                        for (size_type i = ii; i < iend; ++i) {
                            for (size_type k = kk; k < kend; ++k) {
                                const result_type aval = static_cast<result_type>((*this)(i, k));
                                for (size_type j = jj; j < jend; ++j) {
                                    result(i, j) += aval * static_cast<result_type>(other(k, j));
                                }
                            }
                        }
                    }
                }
            }
        } else {
            for (size_type i = 0; i < r; ++i) {
                for (size_type j = 0; j < c; ++j) {
                    result(i, j) = result_type{};
                    for (size_type k = 0; k < common; ++k) {
                        result(i, j) += static_cast<result_type>((*this)(i, k)) * static_cast<result_type>(other(k, j));
                    }
                }
            }
        }
        return result;
    }

    // Транспонирование матрицы. Для статических матриц noexcept.
    auto transpose() const noexcept(is_static) {
        using result_type = Matrix<T, Cols, Rows, Allocator>;
        result_type result;
        if constexpr (!result_type::is_static) {
            result.resize(cols(), rows());
        }
        for (size_type i = 0; i < rows(); ++i) {
            for (size_type j = 0; j < cols(); ++j) {
                result(j, i) = (*this)(i, j);
            }
        }
        return result;
    }

    // След матрицы. Доступен только для квадратных матриц.
    T trace() const {
        if (rows() != cols()) {
            throw std::logic_error("Trace is defined only for square matrices");
        }
        T sum = T{};
        for (size_type i = 0; i < rows(); ++i) {
            sum += (*this)(i, i);
        }
        return sum;
    }

    T determinant() const {
        if (rows() != cols()) {
            throw std::logic_error("Determinant is defined only for square matrices");
        }
        const size_type n = rows();
        if (n == 0) {
            return T{1};
        }
        Matrix temp = *this;
        using calc_type = std::common_type_t<T, double>;
        calc_type det = 1;
        int sign = 1;

        for (size_type k = 0; k < n; ++k) {
            size_type pivot = k;
            calc_type max_value = detail::abs_value(static_cast<calc_type>(temp(k, k)));
            for (size_type i = k + 1; i < n; ++i) {
                const calc_type current = detail::abs_value(static_cast<calc_type>(temp(i, k)));
                if (current > max_value) {
                    max_value = current;
                    pivot = i;
                }
            }
            if (max_value == calc_type{}) {
                return T{};
            }
            if (pivot != k) {
                for (size_type j = k; j < n; ++j) {
                    std::swap(temp(k, j), temp(pivot, j));
                }
                sign = -sign;
            }
            det *= static_cast<calc_type>(temp(k, k));
            for (size_type i = k + 1; i < n; ++i) {
                const calc_type factor = static_cast<calc_type>(temp(i, k)) / static_cast<calc_type>(temp(k, k));
                for (size_type j = k + 1; j < n; ++j) {
                    temp(i, j) -= static_cast<T>(factor * static_cast<calc_type>(temp(k, j)));
                }
            }
        }
        return static_cast<T>(det * sign);
    }

    // Обратная матрица, вычисляется через приведение к расширенной матрице и
    // гауссово-жорданово исключение. Бросает std::domain_error для вырожденных.
    Matrix inverse() const {
        if (rows() != cols()) {
            throw std::logic_error("Inverse is defined only for square matrices");
        }
        const size_type n = rows();
        if (n == 0) {
            return Matrix{};
        }
        using calc_type = std::common_type_t<T, double>;
        Matrix<calc_type, Dynamic, Dynamic> augmented(n, 2 * n);
        for (size_type i = 0; i < n; ++i) {
            for (size_type j = 0; j < n; ++j) {
                augmented(i, j) = static_cast<calc_type>((*this)(i, j));
            }
            for (size_type j = 0; j < n; ++j) {
                augmented(i, n + j) = (i == j) ? calc_type{1} : calc_type{0};
            }
        }

        for (size_type k = 0; k < n; ++k) {
            size_type pivot = k;
            calc_type max_value = detail::abs_value(augmented(k, k));
            for (size_type i = k + 1; i < n; ++i) {
                const calc_type current = detail::abs_value(augmented(i, k));
                if (current > max_value) {
                    max_value = current;
                    pivot = i;
                }
            }
            if (max_value == calc_type{}) {
                throw std::domain_error("Matrix is singular and cannot be inverted");
            }
            if (pivot != k) {
                for (size_type j = 0; j < 2 * n; ++j) {
                    std::swap(augmented(k, j), augmented(pivot, j));
                }
            }
            const calc_type pivot_value = augmented(k, k);
            for (size_type j = 0; j < 2 * n; ++j) {
                augmented(k, j) /= pivot_value;
            }
            for (size_type i = 0; i < n; ++i) {
                if (i == k) {
                    continue;
                }
                const calc_type factor = augmented(i, k);
                for (size_type j = 0; j < 2 * n; ++j) {
                    augmented(i, j) -= factor * augmented(k, j);
                }
            }
        }

        Matrix result;
        if constexpr (!decltype(result)::is_static) {
            result.resize(n, n);
        }
        for (size_type i = 0; i < n; ++i) {
            for (size_type j = 0; j < n; ++j) {
                result(i, j) = static_cast<T>(augmented(i, n + j));
            }
        }
        return result;
    }

    void resize(size_type rows, size_type cols) requires (!is_static) {
        set_size(rows, cols);
        data_.assign(rows_ * cols_, T{});
    }

    void reserve(size_type capacity) requires (!is_static) {
        data_.reserve(capacity);
    }

    void shrink_to_fit() requires (!is_static) {
        data_.shrink_to_fit();
    }

    void swap(Matrix& other) noexcept(std::is_nothrow_swappable_v<storage_type>) {
        using std::swap;
        swap(data_, other.data_);
        if constexpr (!is_static) {
            swap(rows_, other.rows_);
            swap(cols_, other.cols_);
        }
    }

    static Matrix zeros() {
        if constexpr (!is_static) {
            static_assert(is_static, "zeros() requires static dimensions or explicit size");
        }
        Matrix result;
        if constexpr (is_static) {
            std::fill(result.data(), result.data() + result.size(), T{});
        }
        return result;
    }

    static Matrix zeros(size_type rows, size_type cols) requires (!is_static) {
        Matrix result(rows, cols);
        return result;
    }

    static Matrix ones() {
        if constexpr (!is_static) {
            static_assert(is_static, "ones() requires static dimensions or explicit size");
        }
        Matrix result;
        std::fill(result.data(), result.data() + result.size(), T{1});
        return result;
    }

    static Matrix ones(size_type rows, size_type cols) requires (!is_static) {
        Matrix result(rows, cols);
        std::fill(result.data(), result.data() + result.size(), T{1});
        return result;
    }

    static Matrix fill(const T& value) {
        if constexpr (!is_static) {
            static_assert(is_static, "fill() requires static dimensions or explicit size");
        }
        Matrix result;
        std::fill(result.data(), result.data() + result.size(), value);
        return result;
    }

    static Matrix fill(size_type rows, size_type cols, const T& value) requires (!is_static) {
        Matrix result(rows, cols);
        std::fill(result.data(), result.data() + result.size(), value);
        return result;
    }

    static Matrix identity() {
        if constexpr (!is_static) {
            static_assert(is_static, "identity() requires static square dimensions");
        }
        if constexpr (Rows != Cols) {
            static_assert(Rows == Cols, "identity() requires square dimensions");
        }
        Matrix result;
        for (size_type i = 0; i < Rows; ++i) {
            for (size_type j = 0; j < Cols; ++j) {
                result(i, j) = (i == j) ? T{1} : T{};
            }
        }
        return result;
    }

    static Matrix identity(size_type n) requires (!is_static) {
        Matrix result(n, n);
        for (size_type i = 0; i < n; ++i) {
            for (size_type j = 0; j < n; ++j) {
                result(i, j) = (i == j) ? T{1} : T{};
            }
        }
        return result;
    }

    // Создаёт диагональную матрицу из списка значений диагонали.
    /**
     * Создаёт диагональную матрицу из списка значений диагонали.
     * Для динамических матриц размер устанавливается по длине списка.
     */
    static Matrix from_diag(std::initializer_list<T> diag) {
        const size_type n = diag.size();
        if constexpr (has_static_rows && has_static_cols) {
            if (Rows != Cols || Rows != n) {
                throw std::invalid_argument("from_diag requires square dimensions matching initializer size");
            }
        }
        Matrix result;
        if constexpr (!is_static) {
            result.resize(n, n);
        }
        for (size_type i = 0; i < n; ++i) {
            for (size_type j = 0; j < n; ++j) {
                result(i, j) = (i == j) ? *(diag.begin() + i) : T{};
            }
        }
        return result;
    }

    /**
     * Создаёт диагональную динамическую матрицу размером n x n,
     * заполняя диагональ одним значением.
     */
    static Matrix from_diag(size_type n, const T& value) requires (!is_static) {
        Matrix result(n, n);
        for (size_type i = 0; i < n; ++i) {
            for (size_type j = 0; j < n; ++j) {
                result(i, j) = (i == j) ? value : T{};
            }
        }
        return result;
    }

    /**
     * Сравнение матриц на равенство.
     * Для плавающих типов используется сравнение с допустимой погрешностью.
     */
    bool operator==(const Matrix& other) const {
        if (rows() != other.rows() || cols() != other.cols()) {
            return false;
        }
        for (size_type i = 0; i < size(); ++i) {
            if constexpr (std::is_floating_point_v<T>) {
                if (detail::abs_value(data()[i] - other.data()[i]) > epsilon) {
                    return false;
                }
            } else {
                if (data()[i] != other.data()[i]) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Лексикографическое сравнение матриц.
     * Сначала сравниваются размеры, затем элементы по порядку.
     */
    auto operator<=>(const Matrix& other) const {
        if (rows() != other.rows()) {
            if (rows() < other.rows()) {
                return std::strong_ordering::less;
            }
            return std::strong_ordering::greater;
        }
        if (cols() != other.cols()) {
            if (cols() < other.cols()) {
                return std::strong_ordering::less;
            }
            return std::strong_ordering::greater;
        }
        for (size_type i = 0; i < size(); ++i) {
            if constexpr (detail::ThreeWayComparable<T>) {
                auto cmp = data()[i] <=> other.data()[i];
                if (cmp != 0) {
                    return cmp;
                }
            } else {
                if (data()[i] < other.data()[i]) {
                    return std::strong_ordering::less;
                }
                if (data()[i] > other.data()[i]) {
                    return std::strong_ordering::greater;
                }
            }
        }
        return std::strong_ordering::equal;
    }

    /**
     * Записывает матрицу в бинарном формате:
     * размеры в виде двух uint64_t, затем данные элементов.
     */
    void write_binary(std::ostream& os) const {
        const uint64_t r = static_cast<uint64_t>(rows());
        const uint64_t c = static_cast<uint64_t>(cols());
        os.write(reinterpret_cast<const char*>(&r), sizeof(r));
        os.write(reinterpret_cast<const char*>(&c), sizeof(c));
        if (size()) {
            os.write(reinterpret_cast<const char*>(data()), sizeof(T) * size());
        }
    }

    /**
     * Загружает матрицу из бинарного формата.
     * Для статической матрицы размеры должны совпадать.
     */
    void read_binary(std::istream& is) {
        uint64_t r = 0;
        uint64_t c = 0;
        is.read(reinterpret_cast<char*>(&r), sizeof(r));
        is.read(reinterpret_cast<char*>(&c), sizeof(c));
        if (!is) {
            throw std::runtime_error("Failed to read binary matrix header");
        }
        if constexpr (is_static) {
            if (static_cast<size_type>(r) != rows() || static_cast<size_type>(c) != cols()) {
                throw std::invalid_argument("Binary matrix dimensions do not match static matrix shape");
            }
        }
        if constexpr (!is_static) {
            resize(static_cast<size_type>(r), static_cast<size_type>(c));
        }
        is.read(reinterpret_cast<char*>(data()), sizeof(T) * size());
        if (!is) {
            throw std::runtime_error("Failed to read binary matrix data");
        }
    }

private:
    // Хранилище элементов матрицы. Статические матрицы используют std::array,
    // динамические — std::vector с аллокатором.
    using storage_type = std::conditional_t<is_static, std::array<T, Rows * Cols>, std::vector<T, Allocator>>;
    storage_type data_{};
    size_type rows_ = has_static_rows ? Rows : 0;
    size_type cols_ = has_static_cols ? Cols : 0;

    /**
     * Вспомогательный метод для установки размеров динамической матрицы.
     * Проверяет соответствие статических размеров, если они заданы.
     */
    void set_size(size_type rows, size_type cols) {
        if constexpr (has_static_rows) {
            if (rows != Rows) {
                throw std::invalid_argument("Fixed row count does not match construction size");
            }
        }
        if constexpr (has_static_cols) {
            if (cols != Cols) {
                throw std::invalid_argument("Fixed column count does not match construction size");
            }
        }
        if constexpr (!is_static) {
            rows_ = rows;
            cols_ = cols;
        }
    }

    /**
     * Инициализирует память для динамической матрицы и заполняет нулями.
     */
    void init_storage() {
        if constexpr (!is_static) {
            data_.assign(rows_ * cols_, T{});
        }
    }

    /**
     * Заполняет матрицу результатом выражения, проверяя размерность.
     */
    template<MatrixExpression E>
    void assign_expression(const E& expression) {
        if constexpr (has_static_rows) {
            if (expression.rows() != Rows) {
                throw std::invalid_argument("Expression shape mismatch for assignment");
            }
        }
        if constexpr (has_static_cols) {
            if (expression.cols() != Cols) {
                throw std::invalid_argument("Expression shape mismatch for assignment");
            }
        }
        if constexpr (!is_static) {
            set_size(expression.rows(), expression.cols());
            data_.assign(rows_ * cols_, T{});
        }
        for (size_type i = 0; i < rows(); ++i) {
            for (size_type j = 0; j < cols(); ++j) {
                (*this)(i, j) = static_cast<T>(expression(i, j));
            }
        }
    }

    /**
     * Конвертирующий конструктор/присваивание из другой матрицы с совместимыми типами.
     */
    template<Arithmetic U, std::size_t R2, std::size_t C2, typename Alloc2>
    void assign_from(const Matrix<U, R2, C2, Alloc2>& other) {
        if constexpr (has_static_rows) {
            if (other.rows() != Rows) {
                throw std::invalid_argument("Matrix conversion row count mismatch");
            }
        }
        if constexpr (has_static_cols) {
            if (other.cols() != Cols) {
                throw std::invalid_argument("Matrix conversion column count mismatch");
            }
        }
        if constexpr (!is_static) {
            set_size(other.rows(), other.cols());
            data_.assign(rows_ * cols_, T{});
        }
        for (size_type i = 0; i < rows(); ++i) {
            for (size_type j = 0; j < cols(); ++j) {
                (*this)(i, j) = static_cast<T>(other(i, j));
            }
        }
    }

    /**
     * Проверяет, что две матрицы имеют одинаковую размерность.
     * Выбрасывает std::invalid_argument при несоответствии.
     */
    template<MatrixExpression E>
    void ensure_same_shape(const E& other) const {
        if (rows() != other.rows() || cols() != other.cols()) {
            throw std::invalid_argument("Matrix shapes do not match");
        }
    }
};

// Оператор вывода матрицы в текстовом формате.
// Каждая строка матрицы выводится на новой строке, элементы разделены пробелом.
template<Arithmetic T, std::size_t Rows, std::size_t Cols, typename Alloc>
std::ostream& operator<<(std::ostream& os, const Matrix<T, Rows, Cols, Alloc>& matrix) {
    for (std::size_t i = 0; i < matrix.rows(); ++i) {
        for (std::size_t j = 0; j < matrix.cols(); ++j) {
            os << matrix(i, j);
            if (j + 1 < matrix.cols()) {
                os << ' ';
            }
        }
        if (i + 1 < matrix.rows()) {
            os << '\n';
        }
    }
    return os;
}

template<Arithmetic T, std::size_t Rows, std::size_t Cols, typename Alloc>
std::istream& operator>>(std::istream& is, Matrix<T, Rows, Cols, Alloc>& matrix) {
    if constexpr (!matrix.is_static) {
        std::vector<T> values;
        std::string line;
        std::size_t cols = 0;
        std::size_t rows = 0;

        while (std::getline(is, line)) {
            if (line.empty()) {
                break;
            }
            std::istringstream row_stream(line);
            T value;
            std::size_t row_cols = 0;
            while (row_stream >> value) {
                values.push_back(value);
                ++row_cols;
            }
            if (row_cols == 0) {
                continue;
            }
            if (cols == 0) {
                cols = row_cols;
            } else if (row_cols != cols) {
                is.setstate(std::ios::failbit);
                return is;
            }
            ++rows;
        }
        matrix.resize(rows, cols);
        for (std::size_t i = 0; i < values.size(); ++i) {
            matrix.data()[i] = values[i];
        }
        return is;
    } else {
        for (std::size_t i = 0; i < matrix.rows(); ++i) {
            for (std::size_t j = 0; j < matrix.cols(); ++j) {
                if (!(is >> matrix(i, j))) {
                    return is;
                }
            }
        }
        return is;
    }
}

#ifdef __cpp_lib_format
} // namespace math

template<math::Arithmetic T, std::size_t Rows, std::size_t Cols, typename Allocator>
struct std::formatter<math::Matrix<T, Rows, Cols, Allocator>> {
    std::formatter<T> value_formatter;

    auto parse(std::format_parse_context& ctx) {
        return value_formatter.parse(ctx);
    }

    auto format(const math::Matrix<T, Rows, Cols, Allocator>& matrix, std::format_context& ctx) {
        for (std::size_t i = 0; i < matrix.rows(); ++i) {
            for (std::size_t j = 0; j < matrix.cols(); ++j) {
                value_formatter.format(matrix(i, j), ctx);
                if (j + 1 < matrix.cols()) {
                    format_to(ctx.out(), " ");
                }
            }
            if (i + 1 < matrix.rows()) {
                format_to(ctx.out(), "\n");
            }
        }
        return ctx.out();
    }
};
#else
} // namespace math
#endif

#endif // MATH_MATRIX_HPP

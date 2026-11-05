// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_eigen.h>
#include <algorithm>
#include <climits>
#include <cmath>
#include <limits>

namespace numeric {
namespace {
class ArrayScale : public Operation {
    std::vector<double> scratch_;
public:
    void reset() override { std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 2 || no != 1) return {Status::InvalidArgument, "array scale requires array and factor"};
        const Input& x = in[0]; const Input& factor = in[1]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count || x.count > INT_MAX || x.stride != 1 || x.count > x.capacity ||
            factor.type != Type::Float64 || factor.shape != Shape::Scalar || !factor.data || factor.count != 1 || !factor.capacity || factor.stride != 1 ||
            !std::isfinite(factor.data[0]) || y.type != Type::Float64 || y.shape != Shape::Array || !y.data || y.capacity < x.count)
            return {Status::InvalidArgument, "invalid array scale buffers or factor"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "array values must be finite"};
        try { scratch_.assign(x.data, x.data+x.count); }
        catch (...) { return {Status::NumericalError, "scale workspace allocation failed"}; }
        gsl_vector_view vector = gsl_vector_view_array(scratch_.data(), x.count);
        if (gsl_vector_scale(&vector.vector, factor.data[0]) != GSL_SUCCESS) return {Status::NumericalError, "GSL vector scaling failed"};
        for (double value : scratch_) if (!std::isfinite(value)) return {Status::NumericalError, "non-finite scaled value"};
        for (std::size_t i = 0; i < x.count; ++i) y.data[i] = scratch_[i];
        y.count = x.count; return {Status::Ok, "ok"};
    }
};
class SymmetricEigenvalues : public Operation {
    std::vector<double> matrix_, values_;
    std::unique_ptr<gsl_eigen_symm_workspace, decltype(&gsl_eigen_symm_free)> workspace_{nullptr, &gsl_eigen_symm_free};
    std::size_t dimension_ = 0;
public:
    void reset() override { workspace_.reset(); dimension_ = 0; std::vector<double>().swap(matrix_); std::vector<double>().swap(values_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 2 || no != 1) return {Status::InvalidArgument, "eigenvalues require matrix and dimension"};
        const Input& x = in[0]; const Input& d = in[1]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.stride != 1 || x.count > x.capacity ||
            d.type != Type::Float64 || d.shape != Shape::Scalar || !d.data || d.count != 1 || !d.capacity || !d.stride ||
            y.type != Type::Float64 || y.shape != Shape::Array || !y.data)
            return {Status::InvalidArgument, "invalid eigenvalue buffers"};
        const double dim = d.data[0];
        if (!std::isfinite(dim) || dim < 1 || dim > 1024 || std::floor(dim) != dim) return {Status::InvalidArgument, "invalid matrix dimension"};
        const std::size_t n = static_cast<std::size_t>(dim);
        if (x.count != n*n || y.capacity < n) return {Status::InvalidArgument, "invalid matrix or output size"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "matrix must be finite"};
        for (std::size_t i = 0; i < n; ++i) for (std::size_t j = 0; j < i; ++j)
            if (x.data[i*n+j] != x.data[j*n+i]) return {Status::InvalidArgument, "matrix must be symmetric"};
        if (dimension_ != n) {
            std::unique_ptr<gsl_eigen_symm_workspace, decltype(&gsl_eigen_symm_free)> fresh(gsl_eigen_symm_alloc(n), &gsl_eigen_symm_free);
            if (!fresh) return {Status::NumericalError, "eigenvalue workspace allocation failed"};
            workspace_ = std::move(fresh); dimension_ = n;
        }
        try { matrix_.assign(x.data, x.data + x.count); values_.resize(n); }
        catch (...) { return {Status::NumericalError, "eigenvalue buffer allocation failed"}; }
        gsl_matrix_view matrix = gsl_matrix_view_array(matrix_.data(), n, n);
        gsl_vector_view values = gsl_vector_view_array(values_.data(), n);
        if (gsl_eigen_symm(&matrix.matrix, &values.vector, workspace_.get()) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL eigenvalue calculation failed"};
        for (double value : values_) if (!std::isfinite(value)) return {Status::NumericalError, "non-finite eigenvalue"};
        std::sort(values_.begin(), values_.end());
        for (std::size_t i = 0; i < n; ++i) y.data[i] = values_[i];
        y.count = n; return {Status::Ok, "ok"};
    }
};
class MatrixDeterminant : public Operation {
    std::vector<double> scratch_;
    std::unique_ptr<gsl_permutation, decltype(&gsl_permutation_free)> permutation_{nullptr, &gsl_permutation_free};
    std::size_t dimension_ = 0;
public:
    void reset() override { permutation_.reset(); dimension_ = 0; std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 2 || no != 1) return {Status::InvalidArgument, "determinant requires matrix and dimension"};
        const Input& x = in[0]; const Input& d = in[1]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.stride != 1 || x.count > x.capacity ||
            d.type != Type::Float64 || d.shape != Shape::Scalar || !d.data || d.count != 1 || !d.capacity || !d.stride ||
            y.type != Type::Float64 || y.shape != Shape::Scalar || !y.data || !y.capacity)
            return {Status::InvalidArgument, "invalid determinant buffers"};
        const double dim = d.data[0];
        if (!std::isfinite(dim) || dim < 1 || dim > 1024 || std::floor(dim) != dim) return {Status::InvalidArgument, "invalid matrix dimension"};
        const std::size_t n = static_cast<std::size_t>(dim);
        if (x.count != n*n) return {Status::InvalidArgument, "matrix must contain dimension squared values"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "matrix must be finite"};
        if (dimension_ != n) {
            std::unique_ptr<gsl_permutation, decltype(&gsl_permutation_free)> fresh(gsl_permutation_alloc(n), &gsl_permutation_free);
            if (!fresh) return {Status::NumericalError, "permutation allocation failed"};
            permutation_ = std::move(fresh); dimension_ = n;
        }
        try { scratch_.assign(x.data, x.data + x.count); }
        catch (...) { return {Status::NumericalError, "matrix workspace allocation failed"}; }
        gsl_matrix_view matrix = gsl_matrix_view_array(scratch_.data(), n, n);
        int sign = 0; const int status = gsl_linalg_LU_decomp(&matrix.matrix, permutation_.get(), &sign);
        bool zeroDiagonal = false;
        for (double element : scratch_) if (!std::isfinite(element)) return {Status::NumericalError, "non-finite LU decomposition"};
        for (std::size_t i = 0; i < n; ++i) if (scratch_[i*n+i] == 0) zeroDiagonal = true;
        if (status != GSL_SUCCESS && !zeroDiagonal) return {Status::NumericalError, "GSL LU decomposition failed"};
        const double value = gsl_linalg_LU_det(&matrix.matrix, sign);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite determinant"};
        y.data[0] = value; y.count = 1; return {Status::Ok, "ok"};
    }
};
class VectorNorm : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1)
            return {Status::InvalidArgument, "vector_norm requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count ||
            x.count > INT_MAX || !x.stride || x.stride > INT_MAX ||
            (x.count > 1 && x.count > static_cast<std::size_t>(INT_MAX) / x.stride) ||
            (x.count - 1) > (std::numeric_limits<std::size_t>::max() - 1) / x.stride ||
            (x.count - 1) * x.stride >= x.capacity || y.type != Type::Float64 ||
            y.shape != Shape::Scalar || !y.data || !y.capacity)
            return {Status::InvalidArgument, "invalid vector buffer, stride, count or output"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i * x.stride])) return {Status::InvalidArgument, "vector elements must be finite"};
        const gsl_vector_const_view view = gsl_vector_const_view_array_with_stride(x.data, x.stride, x.count);
        const double value = gsl_blas_dnrm2(&view.vector);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite vector norm"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeVectorNorm() { return std::unique_ptr<Operation>(new VectorNorm); }
std::unique_ptr<Operation> makeMatrixDeterminant() { return std::unique_ptr<Operation>(new MatrixDeterminant); }
std::unique_ptr<Operation> makeSymmetricEigenvalues() { return std::unique_ptr<Operation>(new SymmetricEigenvalues); }
std::unique_ptr<Operation> makeArrayScale() { return std::unique_ptr<Operation>(new ArrayScale); }
}
void registerLinearAlgebra(Registry& r) {
    const ArgumentValue defaultFactor{ArgumentType::Floating, 1.0, 0};
    ArgumentDescriptor factor{"factor", ArgumentType::Floating, false, true, defaultFactor, false, false, defaultFactor, defaultFactor, {}};
    factor.inputIndex = 1;
    Metadata scale{"array_scale", "linear algebra", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Array}}, "finite scale factor; default 1", false,
                   "per-instance reusable scratch; reset frees it", "invalid input, allocation or numerical error"};
    scale.arguments = {factor}; scale.argumentSchemaComplete = true; r.add(scale, &makeArrayScale);
    r.addWithoutArguments(Metadata{"matrix_eigenvalues_symmetric", "linear algebra", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Array}}, "row-major symmetric matrix and dimension 1..1024; sorted real eigenvalues", false,
                   "per-instance reusable matrix and GSL workspace; reset frees them", "invalid matrix, allocation or numerical error"}, &makeSymmetricEigenvalues);
    r.addWithoutArguments(Metadata{"matrix_determinant", "linear algebra", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "row-major square matrix and dimension 1..1024", false,
                   "per-instance reusable matrix and permutation; reset frees them", "invalid matrix, allocation or numerical error"}, &makeMatrixDeterminant);
    r.addWithoutArguments(Metadata{"vector_norm", "linear algebra", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "Euclidean norm of a nonempty finite vector", false,
                   "per-instance object; no workspace", "invalid vector or non-finite result"}, &makeVectorNorm);
}
}

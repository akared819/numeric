// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_blas.h>
#include <gsl/gsl_vector.h>
#include <climits>
#include <cmath>
#include <limits>

namespace numeric {
namespace {
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
}
void registerLinearAlgebra(Registry& r) {
    r.add(Metadata{"vector_norm", "linear algebra", {{Type::Float64, Shape::Array}},
                   {{Type::Float64, Shape::Scalar}}, "Euclidean norm of a nonempty finite vector", false,
                   "per-instance object; no workspace", "invalid vector or non-finite result"}, &makeVectorNorm);
}
}

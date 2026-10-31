// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_poly.h>
#include <climits>
#include <cmath>

namespace numeric {
namespace {
class Polynomial : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 2 || no != 1)
            return {Status::InvalidArgument, "polynomial_eval requires coefficients and a scalar argument"};
        const Input& c = in[0]; const Input& x = in[1]; Output& y = out[0];
        if (c.type != Type::Float64 || c.shape != Shape::Array || !c.data || !c.count ||
            c.count > INT_MAX || c.stride != 1 || c.count > c.capacity ||
            x.type != Type::Float64 || x.shape != Shape::Scalar || !x.data ||
            x.count != 1 || x.capacity < 1 || x.stride != 1 ||
            y.type != Type::Float64 || y.shape != Shape::Scalar || !y.data || !y.capacity || !std::isfinite(x.data[0]))
            return {Status::InvalidArgument, "invalid polynomial inputs or output"};
        for (std::size_t i = 0; i < c.count; ++i)
            if (!std::isfinite(c.data[i])) return {Status::InvalidArgument, "coefficients must be finite"};
        const double value = gsl_poly_eval(c.data, static_cast<int>(c.count), x.data[0]);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite polynomial result"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makePolynomial() { return std::unique_ptr<Operation>(new Polynomial); }
}
void registerPolynomial(Registry& r) {
    r.add(Metadata{"polynomial_eval", "polynomials", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "coefficients in ascending power order and a finite evaluation point", false,
                   "per-instance object; no workspace", "invalid input or non-finite result"}, &makePolynomial);
}
}

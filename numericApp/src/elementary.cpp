// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_math.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_sf_pow_int.h>
#include <cmath>

namespace numeric {
namespace {
class LogOnePlus : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "log1p requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Scalar || !x.data || x.count != 1 ||
            x.capacity < 1 || x.stride != 1 || y.type != Type::Float64 || y.shape != Shape::Scalar ||
            !y.data || y.capacity < 1 || !std::isfinite(x.data[0]) || x.data[0] <= -1.0)
            return {Status::InvalidArgument, "log1p requires finite x greater than -1 and valid scalar buffers"};
        const double value = gsl_log1p(x.data[0]);
        if (!std::isfinite(value)) return {Status::NumericalError, "non-finite GSL log1p result"};
        y.data[0] = value; y.count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeLogOnePlus() { return std::unique_ptr<Operation>(new LogOnePlus); }
class IntegerPower : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 2 || no != 1) return {Status::InvalidArgument, "pow_int requires base and exponent"};
        for (std::size_t i = 0; i < 2; ++i)
            if (in[i].type != Type::Float64 || in[i].shape != Shape::Scalar || !in[i].data || in[i].count != 1 ||
                in[i].capacity < 1 || in[i].stride != 1 || !std::isfinite(in[i].data[0]))
                return {Status::InvalidArgument, "invalid power input"};
        if (out[0].type != Type::Float64 || out[0].shape != Shape::Scalar || !out[0].data || out[0].capacity < 1)
            return {Status::InvalidArgument, "invalid power output"};
        const double base = in[0].data[0], exponent = in[1].data[0];
        if (exponent < -1024 || exponent > 1024 || std::floor(exponent) != exponent || (base == 0 && exponent < 0))
            return {Status::InvalidArgument, "invalid power domain or exponent"};
        gsl_sf_result result;
        if (gsl_sf_pow_int_e(base, static_cast<int>(exponent), &result) != GSL_SUCCESS ||
            !std::isfinite(result.val) || (base != 0 && result.val == 0))
            return {Status::NumericalError, "power result overflow or underflow"};
        out[0].data[0] = result.val; out[0].count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeIntegerPower() { return std::unique_ptr<Operation>(new IntegerPower); }
}
void registerElementary(Registry& r) {
    r.addWithoutArguments(Metadata{"log1p", "elementary functions", {{Type::Float64, Shape::Scalar}},
                   {{Type::Float64, Shape::Scalar}}, "finite scalar input greater than -1", false,
                   "per-instance object; no workspace", "invalid input or non-finite result"}, &makeLogOnePlus);
    const Descriptor scalar{Type::Float64, Shape::Scalar};
    const ArgumentValue min{ArgumentType::Integer, 0.0, -1024}, max{ArgumentType::Integer, 0.0, 1024};
    const ArgumentValue defaultExponent{ArgumentType::Integer, 0.0, 2};
    ArgumentDescriptor exponent{"exponent", ArgumentType::Integer, false, true, defaultExponent, true, true, min, max, {}};
    exponent.inputIndex = 1;
    Metadata power{"pow_int", "elementary functions", {scalar, scalar}, {scalar},
                   "finite base; integer exponent -1024..1024; zero to a negative power is invalid", false,
                   "per-instance object; no workspace", "invalid input or result overflow or underflow"};
    power.arguments = {exponent}; power.argumentSchemaComplete = true;
    r.add(power, &makeIntegerPower);
}
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>
#include <cmath>
#include <cstdint>
#include <limits>

namespace numeric {
namespace {
class ComplexFft : public Operation {
    bool inverse_;
public:
    explicit ComplexFft(bool inverse) : inverse_(inverse) {}
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1)
            return {Status::InvalidArgument, "FFT requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array ||
            y.type != Type::Float64 || y.shape != Shape::Array || !x.data || !y.data ||
            x.stride != 1 || x.count < 2 || x.count % 2 || x.count > x.capacity || x.count > y.capacity)
            return {Status::InvalidArgument, "invalid FFT buffer, type, stride or capacity"};
        const std::size_t n = x.count / 2;
        if (n & (n - 1)) return {Status::InvalidArgument, "FFT length must be a power of two"};
        if (x.count > std::numeric_limits<std::uintptr_t>::max() / sizeof(double))
            return {Status::InvalidArgument, "FFT buffer size overflow"};
        const std::uintptr_t bytes = x.count * sizeof(double);
        const std::uintptr_t a = reinterpret_cast<std::uintptr_t>(x.data);
        const std::uintptr_t b = reinterpret_cast<std::uintptr_t>(y.data);
        if (a > std::numeric_limits<std::uintptr_t>::max() - bytes ||
            b > std::numeric_limits<std::uintptr_t>::max() - bytes ||
            (a != b && a < b + bytes && b < a + bytes))
            return {Status::InvalidArgument, "FFT buffers partially overlap or overflow"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "FFT input must be finite"};
        if (a != b) for (std::size_t i = 0; i < x.count; ++i) y.data[i] = x.data[i];
        const int status = inverse_ ? gsl_fft_complex_radix2_inverse(y.data, 1, n)
                                    : gsl_fft_complex_radix2_forward(y.data, 1, n);
        if (status != GSL_SUCCESS) return {Status::NumericalError, "GSL FFT failed"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(y.data[i])) return {Status::NumericalError, "non-finite FFT result"};
        y.count = x.count;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> forward() { return std::unique_ptr<Operation>(new ComplexFft(false)); }
std::unique_ptr<Operation> inverse() { return std::unique_ptr<Operation>(new ComplexFft(true)); }
}
void registerFft(Registry& r) {
    const std::vector<Descriptor> arrays{{Type::Float64, Shape::Array}};
    r.add(Metadata{"fft_complex_radix2_forward", "fft", arrays, arrays,
                   "interleaved complex DOUBLE; power-of-two complex length", false,
                   "per-instance object; no workspace", "invalid buffers or non-finite data/result"}, &forward);
    r.add(Metadata{"fft_complex_radix2_inverse", "fft", arrays, arrays,
                   "interleaved complex DOUBLE; power-of-two complex length; inverse scales by 1/N", false,
                   "per-instance object; no workspace", "invalid buffers or non-finite data/result"}, &inverse);
}
}

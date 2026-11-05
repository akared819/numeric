// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_complex.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <cmath>
#include <cstdint>
#include <limits>

namespace numeric {
namespace {
class MixedComplexFft : public Operation {
    bool inverse_;
    std::size_t length_ = 0;
    std::unique_ptr<gsl_fft_complex_wavetable, decltype(&gsl_fft_complex_wavetable_free)> table_{nullptr, &gsl_fft_complex_wavetable_free};
    std::unique_ptr<gsl_fft_complex_workspace, decltype(&gsl_fft_complex_workspace_free)> work_{nullptr, &gsl_fft_complex_workspace_free};
    std::vector<double> scratch_;
public:
    explicit MixedComplexFft(bool inverse) : inverse_(inverse) {}
    void reset() override { table_.reset(); work_.reset(); length_ = 0; std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "FFT requires one input and output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.count < 2 || x.count % 2 ||
            x.stride != 1 || x.count > x.capacity || y.type != Type::Float64 || y.shape != Shape::Array ||
            !y.data || y.capacity < x.count || x.count > std::numeric_limits<std::size_t>::max() / sizeof(double))
            return {Status::InvalidArgument, "invalid FFT buffers or length"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "FFT input must be finite"};
        const std::size_t n = x.count / 2;
        if (n != length_) {
            std::unique_ptr<gsl_fft_complex_wavetable, decltype(&gsl_fft_complex_wavetable_free)> table(gsl_fft_complex_wavetable_alloc(n), &gsl_fft_complex_wavetable_free);
            if (!table) return {Status::NumericalError, "FFT wavetable allocation failed"};
            std::unique_ptr<gsl_fft_complex_workspace, decltype(&gsl_fft_complex_workspace_free)> work(gsl_fft_complex_workspace_alloc(n), &gsl_fft_complex_workspace_free);
            if (!work) return {Status::NumericalError, "FFT workspace allocation failed"};
            table_ = std::move(table); work_ = std::move(work); length_ = n;
        }
        try { scratch_.assign(x.data, x.data + x.count); }
        catch (...) { return {Status::NumericalError, "FFT scratch allocation failed"}; }
        const int status = inverse_ ? gsl_fft_complex_inverse(scratch_.data(), 1, n, table_.get(), work_.get())
                                    : gsl_fft_complex_forward(scratch_.data(), 1, n, table_.get(), work_.get());
        if (status != GSL_SUCCESS) return {Status::NumericalError, "GSL FFT failed"};
        for (double value : scratch_) if (!std::isfinite(value)) return {Status::NumericalError, "non-finite FFT result"};
        for (std::size_t i = 0; i < x.count; ++i) y.data[i] = scratch_[i];
        y.count = x.count; return {Status::Ok, "ok"};
    }
};
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
class RealInverseFft : public Operation {
    std::vector<double> scratch_;
public:
    void reset() override { std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "real inverse FFT requires one input and output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.count < 2 || x.count % 2 ||
            x.stride != 1 || x.count > x.capacity || y.type != Type::Float64 || y.shape != Shape::Array || !y.data)
            return {Status::InvalidArgument, "invalid inverse FFT buffers"};
        const std::size_t n = x.count / 2;
        if ((n & (n - 1)) || y.capacity < n) return {Status::InvalidArgument, "inverse FFT length or output capacity invalid"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "FFT input must be finite"};
        if (x.data[1] != 0 || (n > 1 && x.data[n + 1] != 0))
            return {Status::InvalidArgument, "DC and Nyquist bins must be real"};
        for (std::size_t k = 1; k < (n + 1) / 2; ++k)
            if (x.data[2*k] != x.data[2*(n-k)] || x.data[2*k+1] != -x.data[2*(n-k)+1])
                return {Status::InvalidArgument, "spectrum must be conjugate symmetric"};
        try { scratch_.resize(n); }
        catch (...) { return {Status::NumericalError, "inverse FFT scratch allocation failed"}; }
        scratch_[0] = x.data[0];
        if (n > 1) scratch_[n/2] = x.data[n];
        for (std::size_t k = 1; k < (n + 1) / 2; ++k) {
            scratch_[k] = x.data[2*k]; scratch_[n-k] = x.data[2*k+1];
        }
        if (gsl_fft_halfcomplex_radix2_inverse(scratch_.data(), 1, n) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL inverse real FFT failed"};
        for (double value : scratch_) if (!std::isfinite(value)) return {Status::NumericalError, "non-finite inverse FFT result"};
        for (std::size_t i = 0; i < n; ++i) y.data[i] = scratch_[i];
        y.count = n; return {Status::Ok, "ok"};
    }
};
class RealFft : public Operation {
    std::vector<double> scratch_;
public:
    void reset() override { std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1)
            return {Status::InvalidArgument, "real FFT requires one input and one output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count ||
            x.stride != 1 || x.count > x.capacity || y.type != Type::Float64 ||
            y.shape != Shape::Array || !y.data || (x.count & (x.count - 1)) ||
            x.count > std::numeric_limits<std::size_t>::max() / (2 * sizeof(double)) ||
            y.capacity < x.count * 2)
            return {Status::InvalidArgument, "invalid real FFT buffer, length, stride or capacity"};
        for (std::size_t i = 0; i < x.count; ++i)
            if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "real FFT input must be finite"};
        try { scratch_.resize(x.count); }
        catch (...) { return {Status::NumericalError, "real FFT scratch allocation failed"}; }
        for (std::size_t i = 0; i < x.count; ++i) scratch_[i] = x.data[i];
        if (gsl_fft_real_radix2_transform(scratch_.data(), 1, x.count) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL real FFT failed"};
        for (double value : scratch_)
            if (!std::isfinite(value)) return {Status::NumericalError, "non-finite real FFT result"};
        if (gsl_fft_halfcomplex_radix2_unpack(scratch_.data(), y.data, 1, x.count) != GSL_SUCCESS)
            return {Status::NumericalError, "GSL FFT unpack failed"};
        y.count = 2 * x.count;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> forward() { return std::unique_ptr<Operation>(new ComplexFft(false)); }
std::unique_ptr<Operation> mixedForward() { return std::unique_ptr<Operation>(new MixedComplexFft(false)); }
std::unique_ptr<Operation> mixedInverse() { return std::unique_ptr<Operation>(new MixedComplexFft(true)); }
std::unique_ptr<Operation> inverse() { return std::unique_ptr<Operation>(new ComplexFft(true)); }
std::unique_ptr<Operation> realForward() { return std::unique_ptr<Operation>(new RealFft()); }
std::unique_ptr<Operation> realInverse() { return std::unique_ptr<Operation>(new RealInverseFft()); }
}
void registerFft(Registry& r) {
    const std::vector<Descriptor> arrays{{Type::Float64, Shape::Array}};
    r.addWithoutArguments(Metadata{"fft_complex_mixed_forward", "fft", arrays, arrays,
                   "interleaved complex DOUBLE; positive length", false,
                   "per-instance reusable GSL wavetable, workspace and scratch", "invalid buffers or allocation or numerical failure"}, &mixedForward);
    r.addWithoutArguments(Metadata{"fft_complex_mixed_inverse", "fft", arrays, arrays,
                   "interleaved complex DOUBLE; positive length; inverse scales by 1/N", false,
                   "per-instance reusable GSL wavetable, workspace and scratch", "invalid buffers or allocation or numerical failure"}, &mixedInverse);
    r.addWithoutArguments(Metadata{"fft_complex_radix2_forward", "fft", arrays, arrays,
                   "interleaved complex DOUBLE; power-of-two complex length", false,
                   "per-instance object; no workspace", "invalid buffers or non-finite data/result"}, &forward);
    r.addWithoutArguments(Metadata{"fft_complex_radix2_inverse", "fft", arrays, arrays,
                   "interleaved complex DOUBLE; power-of-two complex length; inverse scales by 1/N", false,
                   "per-instance object; no workspace", "invalid buffers or non-finite data/result"}, &inverse);
    r.addWithoutArguments(Metadata{"fft_real_radix2_forward", "fft", arrays, arrays,
                   "real DOUBLE input; power-of-two length; full interleaved complex output", false,
                   "per-instance reusable scratch; reset frees it", "invalid buffers or allocation or numerical failure"}, &realForward);
    r.addWithoutArguments(Metadata{"fft_real_radix2_inverse", "fft", arrays, arrays,
                   "conjugate-symmetric full complex input; power-of-two length; inverse scales by 1/N", false,
                   "per-instance reusable scratch; reset frees it", "invalid buffers or allocation or numerical failure"}, &realInverse);
}
}

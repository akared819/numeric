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
class ShortTimeFft : public Operation {
    std::vector<double> fft_, window_, result_;
public:
    void reset() override { std::vector<double>().swap(fft_); std::vector<double>().swap(window_); std::vector<double>().swap(result_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 3 || no != 1) return {Status::InvalidArgument, "STFT requires samples, frame length and hop"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count || x.stride != 1 || x.count > x.capacity ||
            y.type != Type::Float64 || y.shape != Shape::Array || !y.data) return {Status::InvalidArgument, "invalid STFT buffers"};
        for (std::size_t i = 1; i < 3; ++i)
            if (in[i].type != Type::Float64 || in[i].shape != Shape::Scalar || !in[i].data || in[i].count != 1 || !in[i].capacity || !in[i].stride)
                return {Status::InvalidArgument, "invalid STFT scalar input"};
        const double nd = in[1].data[0], hd = in[2].data[0];
        if (!std::isfinite(nd) || !std::isfinite(hd) || nd < 4 || nd > static_cast<double>(x.count) || nd > 9007199254740991.0 ||
            std::floor(nd) != nd || hd < 1 || hd > nd || std::floor(hd) != hd)
            return {Status::InvalidArgument, "invalid STFT frame length or hop"};
        const std::size_t n = static_cast<std::size_t>(nd), hop = static_cast<std::size_t>(hd);
        if ((n & (n - 1)) || n > std::numeric_limits<std::size_t>::max() / 2) return {Status::InvalidArgument, "invalid STFT frame length"};
        const std::size_t frames = 1 + (x.count - n) / hop, frameValues = 2 * n;
        if (frames > std::numeric_limits<std::size_t>::max() / frameValues || frames * frameValues > y.capacity)
            return {Status::InvalidArgument, "STFT output capacity too small"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "STFT input must be finite"};
        try { fft_.resize(n); window_.resize(n); result_.resize(frames * frameValues); }
        catch (...) { return {Status::NumericalError, "STFT workspace allocation failed"}; }
        const double pi = std::acos(-1.0);
        for (std::size_t i = 0; i < n; ++i) window_[i] = 0.5 * (1.0 - std::cos(2.0*pi*(static_cast<double>(i)/static_cast<double>(n-1))));
        for (std::size_t frame = 0; frame < frames; ++frame) {
            const std::size_t start = frame * hop;
            for (std::size_t i = 0; i < n; ++i) fft_[i] = x.data[start+i] * window_[i];
            if (gsl_fft_real_radix2_transform(fft_.data(), 1, n) != GSL_SUCCESS ||
                gsl_fft_halfcomplex_radix2_unpack(fft_.data(), result_.data() + frame * frameValues, 1, n) != GSL_SUCCESS)
                return {Status::NumericalError, "GSL STFT failed"};
        }
        for (double value : result_) if (!std::isfinite(value)) return {Status::NumericalError, "non-finite STFT result"};
        for (std::size_t i = 0; i < result_.size(); ++i) y.data[i] = result_[i];
        y.count = result_.size(); return {Status::Ok, "ok"};
    }
};
class WelchPsd : public Operation {
    std::vector<double> window_, fft_, result_;
public:
    void reset() override { std::vector<double>().swap(window_); std::vector<double>().swap(fft_); std::vector<double>().swap(result_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 4 || no != 1) return {Status::InvalidArgument, "Welch PSD requires samples, segment length, overlap and sample interval"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count || x.stride != 1 || x.count > x.capacity ||
            y.type != Type::Float64 || y.shape != Shape::Array || !y.data) return {Status::InvalidArgument, "invalid Welch buffers"};
        for (std::size_t i = 1; i < 4; ++i)
            if (in[i].type != Type::Float64 || in[i].shape != Shape::Scalar || !in[i].data || in[i].count != 1 || !in[i].capacity || !in[i].stride)
                return {Status::InvalidArgument, "invalid Welch scalar input"};
        const double nd = in[1].data[0], od = in[2].data[0], dt = in[3].data[0];
        if (!std::isfinite(nd) || !std::isfinite(od) || !std::isfinite(dt) || nd < 4 || nd > static_cast<double>(x.count) ||
            nd > 9007199254740991.0 || std::floor(nd) != nd || od < 0 || od >= nd || std::floor(od) != od || dt <= 0)
            return {Status::InvalidArgument, "invalid Welch parameters"};
        const std::size_t n = static_cast<std::size_t>(nd), overlap = static_cast<std::size_t>(od);
        if ((n & (n - 1)) || y.capacity < n / 2 + 1) return {Status::InvalidArgument, "Welch length or output capacity invalid"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "samples must be finite"};
        try { window_.resize(n); fft_.resize(n); result_.assign(n / 2 + 1, 0.0); }
        catch (...) { return {Status::NumericalError, "Welch workspace allocation failed"}; }
        double sum = 0; const double pi = std::acos(-1.0);
        for (std::size_t i = 0; i < n; ++i) {
            const double w = 0.5 * (1.0 - std::cos(2.0 * pi * (static_cast<double>(i) / static_cast<double>(n - 1))));
            window_[i] = w; sum += w * w;
        }
        const double scaleSquared = dt / sum;
        if (!std::isfinite(scaleSquared) || scaleSquared <= 0) return {Status::NumericalError, "Welch normalization outside floating-point range"};
        const double scale = std::sqrt(scaleSquared);
        const std::size_t step = n - overlap, segments = 1 + (x.count - n) / step;
        for (std::size_t segment = 0; segment < segments; ++segment) {
            const std::size_t start = segment * step;
            for (std::size_t i = 0; i < n; ++i) fft_[i] = x.data[start + i] * window_[i];
            if (gsl_fft_real_radix2_transform(fft_.data(), 1, n) != GSL_SUCCESS) return {Status::NumericalError, "GSL Welch FFT failed"};
            for (std::size_t k = 0; k <= n / 2; ++k) {
                const double magnitude = k == 0 || k == n / 2 ? std::fabs(fft_[k]) : std::hypot(fft_[k], fft_[n-k]);
                const double scaled = magnitude * scale * ((k == 0 || k == n / 2) ? 1.0 : std::sqrt(2.0));
                const double value = scaled * scaled;
                if (!std::isfinite(value)) return {Status::NumericalError, "Welch PSD overflow"};
                result_[k] += (value - result_[k]) / static_cast<double>(segment + 1);
            }
        }
        for (std::size_t k = 0; k < result_.size(); ++k) y.data[k] = result_[k];
        y.count = result_.size(); return {Status::Ok, "ok"};
    }
};
class PowerSpectrum : public Operation {
    std::vector<double> scratch_;
public:
    void reset() override { std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "power spectrum requires one input and output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || x.count < 2 || x.count % 2 ||
            x.stride != 1 || x.count > x.capacity || y.type != Type::Float64 || y.shape != Shape::Array || !y.data || y.capacity < x.count / 2)
            return {Status::InvalidArgument, "invalid spectrum buffers"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "spectrum input must be finite"};
        try { scratch_.resize(x.count / 2); }
        catch (...) { return {Status::NumericalError, "power spectrum scratch allocation failed"}; }
        for (std::size_t i = 0; i < scratch_.size(); ++i) {
            const double magnitude = std::hypot(x.data[2*i], x.data[2*i+1]);
            scratch_[i] = magnitude * magnitude;
            if (!std::isfinite(scratch_[i])) return {Status::NumericalError, "power spectrum overflow"};
        }
        for (std::size_t i = 0; i < scratch_.size(); ++i) y.data[i] = scratch_[i];
        y.count = scratch_.size(); return {Status::Ok, "ok"};
    }
};
class HannWindow : public Operation {
    std::vector<double> scratch_;
public:
    void reset() override { std::vector<double>().swap(scratch_); }
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 1 || no != 1) return {Status::InvalidArgument, "Hann window requires one input and output"};
        const Input& x = in[0]; Output& y = out[0];
        if (x.type != Type::Float64 || x.shape != Shape::Array || !x.data || !x.count || x.stride != 1 ||
            x.count > x.capacity || y.type != Type::Float64 || y.shape != Shape::Array || !y.data || y.capacity < x.count)
            return {Status::InvalidArgument, "invalid Hann window buffers"};
        for (std::size_t i = 0; i < x.count; ++i) if (!std::isfinite(x.data[i])) return {Status::InvalidArgument, "window input must be finite"};
        try { scratch_.resize(x.count); }
        catch (...) { return {Status::NumericalError, "window scratch allocation failed"}; }
        const double pi = std::acos(-1.0);
        for (std::size_t i = 0; i < x.count; ++i) {
            const double weight = x.count == 1 ? 1.0 : 0.5 * (1.0 - std::cos(2.0 * pi * (static_cast<double>(i) / static_cast<double>(x.count - 1))));
            scratch_[i] = x.data[i] * weight;
            if (!std::isfinite(scratch_[i])) return {Status::NumericalError, "non-finite window result"};
        }
        for (std::size_t i = 0; i < x.count; ++i) y.data[i] = scratch_[i];
        y.count = x.count; return {Status::Ok, "ok"};
    }
};
class FrequencyAxis : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out && no == 1) out[0].count = 0;
        if (!in || !out || ni != 2 || no != 1) return {Status::InvalidArgument, "frequency axis requires length and sample interval"};
        for (std::size_t i = 0; i < 2; ++i)
            if (in[i].type != Type::Float64 || in[i].shape != Shape::Scalar || !in[i].data || in[i].count != 1 || !in[i].capacity || !in[i].stride)
                return {Status::InvalidArgument, "invalid scalar input"};
        Output& y = out[0];
        const double length = in[0].data[0], dt = in[1].data[0];
        if (y.type != Type::Float64 || y.shape != Shape::Array || !y.data || !std::isfinite(length) || length < 1 ||
            length > 9007199254740991.0 || std::floor(length) != length ||
            length > static_cast<double>(std::numeric_limits<std::size_t>::max() / sizeof(double)) ||
            !std::isfinite(dt) || dt <= 0)
            return {Status::InvalidArgument, "invalid frequency axis parameters"};
        const std::size_t n = static_cast<std::size_t>(length);
        if (n > y.capacity) return {Status::InvalidArgument, "frequency axis output too small"};
        const double period = length * dt;
        if (!std::isfinite(period) || period <= 0) return {Status::NumericalError, "sample period outside floating-point range"};
        const double spacing = 1.0 / period;
        if (!std::isfinite(spacing) || spacing == 0 || !std::isfinite(static_cast<double>(n / 2) * spacing))
            return {Status::NumericalError, "frequency outside floating-point range"};
        for (std::size_t k = 0; k < n; ++k)
            y.data[k] = k <= (n - 1) / 2 ? static_cast<double>(k) * spacing : -static_cast<double>(n - k) * spacing;
        y.count = n; return {Status::Ok, "ok"};
    }
};
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
std::unique_ptr<Operation> frequencyAxis() { return std::unique_ptr<Operation>(new FrequencyAxis); }
std::unique_ptr<Operation> hannWindow() { return std::unique_ptr<Operation>(new HannWindow); }
std::unique_ptr<Operation> powerSpectrum() { return std::unique_ptr<Operation>(new PowerSpectrum); }
std::unique_ptr<Operation> welchPsd() { return std::unique_ptr<Operation>(new WelchPsd); }
std::unique_ptr<Operation> shortTimeFft() { return std::unique_ptr<Operation>(new ShortTimeFft); }
std::unique_ptr<Operation> inverse() { return std::unique_ptr<Operation>(new ComplexFft(true)); }
std::unique_ptr<Operation> realForward() { return std::unique_ptr<Operation>(new RealFft()); }
std::unique_ptr<Operation> realInverse() { return std::unique_ptr<Operation>(new RealInverseFft()); }
}
void registerFft(Registry& r) {
    const std::vector<Descriptor> arrays{{Type::Float64, Shape::Array}};
    r.addWithoutArguments(Metadata{"stft_hann", "signal processing", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Scalar}, {Type::Float64, Shape::Scalar}}, arrays,
                   "full complex symmetric-Hann STFT; frame length and hop", false,
                   "per-instance reusable vectors; reset frees them", "invalid parameters, allocation or numerical failure"}, &shortTimeFft);
    r.addWithoutArguments(Metadata{"welch_psd", "signal processing", {{Type::Float64, Shape::Array}, {Type::Float64, Shape::Scalar}, {Type::Float64, Shape::Scalar}, {Type::Float64, Shape::Scalar}}, arrays,
                   "one-sided symmetric-Hann Welch PSD; segment length, overlap and sample interval", false,
                   "per-instance reusable vectors; reset frees them", "invalid parameters, allocation or numerical failure"}, &welchPsd);
    r.addWithoutArguments(Metadata{"spectrum_power", "signal processing", arrays, arrays,
                   "squared magnitude of full interleaved complex spectrum; no normalization", false,
                   "per-instance reusable scratch; reset frees it", "invalid buffers or numerical failure"}, &powerSpectrum);
    r.addWithoutArguments(Metadata{"window_hann", "signal processing", arrays, arrays,
                   "symmetric Hann window, single sample unchanged", false,
                   "per-instance reusable scratch; reset frees it", "invalid buffers or numerical failure"}, &hannWindow);
    r.addWithoutArguments(Metadata{"fft_frequency_axis", "fft", {{Type::Float64, Shape::Scalar}, {Type::Float64, Shape::Scalar}}, arrays,
                   "sample count and positive sample interval; signed full frequency axis", false,
                   "per-instance object; no workspace", "invalid parameters, capacity or numeric range"}, &frequencyAxis);
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

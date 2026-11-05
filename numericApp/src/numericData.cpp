// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericData.h"
#include <array>
#include <cmath>
#include <cstring>
#include <limits>

namespace numeric {
namespace {
static_assert(sizeof(float) == 4 && sizeof(double) == 8 && std::numeric_limits<float>::is_iec559 &&
              std::numeric_limits<double>::is_iec559, "numeric typed arrays require IEEE binary32 and binary64");
std::size_t elementSize(ElementType type) {
    switch (type) {
    case ElementType::Float32: return sizeof(float);
    case ElementType::Float64: return sizeof(double);
    case ElementType::Int32: return sizeof(std::int32_t);
    case ElementType::Int64: return sizeof(std::int64_t);
    case ElementType::Complex64: return 2 * sizeof(float);
    case ElementType::Complex128: return 2 * sizeof(double);
    }
    return 0;
}
ArrayView asReadView(const MutableArrayView& v) {
    return {v.data, v.capacityBytes, v.type, v.rank, v.extents, v.stridesBytes};
}
Result checkLayout(const ArrayView& v, bool writable) {
    const std::size_t size = elementSize(v.type);
    if (!size || v.rank > 8 || (v.rank && (!v.extents || !v.stridesBytes)))
        return {Status::InvalidArgument, "invalid array type, rank or shape"};
    for (std::size_t i = 0; i < v.rank; ++i)
        if (!v.extents[i]) return {Status::Ok, "empty array"};
    if (!v.data || v.capacityBytes < size) return {Status::InvalidArgument, "array buffer too small"};
    std::size_t offset = 0;
    std::array<std::size_t, 8> order{}; std::size_t used = 0;
    for (std::size_t i = 0; i < v.rank; ++i) {
        if (v.extents[i] <= 1) continue;
        if (!v.stridesBytes[i] || (v.extents[i] - 1) > (std::numeric_limits<std::size_t>::max() - offset) / v.stridesBytes[i])
            return {Status::InvalidArgument, "invalid stride or array offset overflow"};
        offset += (v.extents[i] - 1) * v.stridesBytes[i];
        std::size_t j = used++;
        while (j && v.stridesBytes[order[j - 1]] > v.stridesBytes[i]) { order[j] = order[j - 1]; --j; }
        order[j] = i;
    }
    if (offset > v.capacityBytes - size) return {Status::InvalidArgument, "array exceeds buffer capacity"};
    if (writable) {
        std::size_t span = size;
        for (std::size_t j = 0; j < used; ++j) {
            const std::size_t i = order[j];
            if (v.stridesBytes[i] < span) return {Status::InvalidArgument, "overlapping writable array"};
            const std::size_t width = (v.extents[i] - 1) * v.stridesBytes[i];
            if (width > std::numeric_limits<std::size_t>::max() - span)
                return {Status::InvalidArgument, "array span overflow"};
            span += width;
        }
    }
    return {Status::Ok, "ok"};
}
Result offsetOf(const ArrayView& v, bool writable, const std::size_t* indices,
                std::size_t count, std::size_t& offset) {
    const Result checked = checkLayout(v, writable);
    if (checked.status != Status::Ok) return checked;
    if (count != v.rank || (count && !indices)) return {Status::InvalidArgument, "invalid index count"};
    offset = 0;
    for (std::size_t i = 0; i < count; ++i) {
        if (indices[i] >= v.extents[i]) return {Status::InvalidArgument, "array index out of range"};
        offset += indices[i] * v.stridesBytes[i];
    }
    return {Status::Ok, "ok"};
}
bool exactFloat(double value, float& converted) {
    if (!std::isfinite(value) || std::fabs(value) > std::numeric_limits<float>::max()) return false;
    converted = static_cast<float>(value);
    return std::isfinite(converted) && static_cast<double>(converted) == value;
}
}
Result validateArray(const ArrayView& view) { return checkLayout(view, false); }
Result validateWritableArray(const MutableArrayView& view) { return checkLayout(asReadView(view), true); }
Result readElement(const ArrayView& view, const std::size_t* indices, std::size_t count, ElementValue& value) {
    std::size_t offset = 0; const Result status = offsetOf(view, false, indices, count, offset);
    if (status.status != Status::Ok) return status;
    const unsigned char* p = static_cast<const unsigned char*>(view.data) + offset;
    ElementValue result{view.type, 0, 0.0, 0.0};
    switch (view.type) {
    case ElementType::Float32: { float x; std::memcpy(&x, p, sizeof(x)); result.real = x; break; }
    case ElementType::Float64: std::memcpy(&result.real, p, sizeof(double)); break;
    case ElementType::Int32: { std::int32_t x; std::memcpy(&x, p, sizeof(x)); result.integer = x; break; }
    case ElementType::Int64: std::memcpy(&result.integer, p, sizeof(result.integer)); break;
    case ElementType::Complex64: { float x[2]; std::memcpy(x, p, sizeof(x)); result.real = x[0]; result.imaginary = x[1]; break; }
    case ElementType::Complex128: { double x[2]; std::memcpy(x, p, sizeof(x)); result.real = x[0]; result.imaginary = x[1]; break; }
    }
    if ((view.type == ElementType::Float32 || view.type == ElementType::Float64 ||
         view.type == ElementType::Complex64 || view.type == ElementType::Complex128) &&
        (!std::isfinite(result.real) || !std::isfinite(result.imaginary)))
        return {Status::InvalidArgument, "non-finite array element"};
    value = result; return {Status::Ok, "ok"};
}
Result writeElement(const MutableArrayView& view, const std::size_t* indices,
                    std::size_t count, const ElementValue& value) {
    std::size_t offset = 0; const Result status = offsetOf(asReadView(view), true, indices, count, offset);
    if (status.status != Status::Ok) return status;
    if (value.type != view.type) return {Status::InvalidArgument, "element type mismatch"};
    unsigned char bytes[16] = {}; const std::size_t size = elementSize(view.type);
    switch (view.type) {
    case ElementType::Float32: { float x; if (!exactFloat(value.real, x)) return {Status::InvalidArgument, "inexact Float32"}; std::memcpy(bytes, &x, sizeof(x)); break; }
    case ElementType::Float64: if (!std::isfinite(value.real)) return {Status::InvalidArgument, "non-finite Float64"}; std::memcpy(bytes, &value.real, sizeof(double)); break;
    case ElementType::Int32: { if (value.integer < std::numeric_limits<std::int32_t>::min() || value.integer > std::numeric_limits<std::int32_t>::max()) return {Status::InvalidArgument, "Int32 out of range"}; const std::int32_t x = static_cast<std::int32_t>(value.integer); std::memcpy(bytes, &x, sizeof(x)); break; }
    case ElementType::Int64: std::memcpy(bytes, &value.integer, sizeof(value.integer)); break;
    case ElementType::Complex64: { float x[2]; if (!exactFloat(value.real, x[0]) || !exactFloat(value.imaginary, x[1])) return {Status::InvalidArgument, "inexact Complex64"}; std::memcpy(bytes, x, sizeof(x)); break; }
    case ElementType::Complex128: { if (!std::isfinite(value.real) || !std::isfinite(value.imaginary)) return {Status::InvalidArgument, "non-finite Complex128"}; const double x[] = {value.real, value.imaginary}; std::memcpy(bytes, x, sizeof(x)); break; }
    }
    std::memcpy(static_cast<unsigned char*>(view.data) + offset, bytes, size);
    return {Status::Ok, "ok"};
}
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericTypedMean.h"
#include "numericDataInternal.h"
#include <gsl/gsl_statistics_double.h>
#include <array>
#include <cmath>
#include <limits>
#include <new>
#include <stdexcept>
namespace numeric {
TypedMean::TypedMean(std::size_t maxElements) : maxElements_(maxElements) {}
Result TypedMean::reserve() {
    if (!maxElements_ || maxElements_ > workspace_.max_size()) return {Status::InvalidArgument, "invalid maximum element count"};
    try { workspace_.reserve(maxElements_); }
    catch (const std::bad_alloc&) { return {Status::NumericalError, "mean workspace allocation failed"}; }
    catch (const std::length_error&) { return {Status::InvalidArgument, "mean workspace too large"}; }
    return {Status::Ok, "ok"};
}
void TypedMean::reset() { std::vector<double>().swap(workspace_); }
Result TypedMean::execute(const ArrayView& input, double& output) {
    if (!isGslInitialized()) return {Status::NotInitialized, "GSL policy not initialized"};
    const Result valid = validateArray(input);
    if (valid.status != Status::Ok) return valid;
    std::size_t count = 1;
    for (std::size_t i = 0; i < input.rank; ++i) {
        if (!input.extents[i]) return {Status::InvalidArgument, "mean input is empty"};
        if (input.extents[i] > std::numeric_limits<std::size_t>::max() / count)
            return {Status::InvalidArgument, "mean element count overflow"};
        count *= input.extents[i];
    }
    if (!maxElements_ || count > maxElements_ || count > workspace_.max_size())
        return {Status::InvalidArgument, "mean input exceeds configured limit"};
    try { workspace_.resize(count); }
    catch (const std::bad_alloc&) { return {Status::NumericalError, "mean workspace allocation failed"}; }
    catch (const std::length_error&) { return {Status::InvalidArgument, "mean workspace too large"}; }
    std::array<std::size_t, 8> indices{};
    for (std::size_t i = 0; i < count; ++i) {
        ElementValue element{}, converted{};
        std::size_t offset = 0;
        for (std::size_t axis = 0; axis < input.rank; ++axis) offset += indices[axis] * input.stridesBytes[axis];
        const Result read = detail::readValidatedElement(input, offset, element);
        if (read.status != Status::Ok) return read;
        const Result conversion = convertElement(element, ElementType::Float64, converted);
        if (conversion.status != Status::Ok) return conversion;
        workspace_[i] = converted.real;
        for (std::size_t j = input.rank; j > 0; --j) {
            const std::size_t axis = j - 1;
            if (++indices[axis] < input.extents[axis]) break;
            indices[axis] = 0;
        }
    }
    const double result = gsl_stats_mean(workspace_.data(), 1, count);
    if (!std::isfinite(result)) return {Status::NumericalError, "non-finite mean result"};
    output = result; return {Status::Ok, "ok"};
}
}

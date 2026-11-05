// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericLinkInput.h"
#include "numericData.h"
#include <dbAccess.h>
#include <dbFldTypes.h>
#include <cmath>

void NumericLinkInput::configure(std::size_t capacity) { values_.resize(capacity); }
numeric::Result NumericLinkInput::read(DBLINK& link, numeric::Shape shape, std::size_t maximum,
                                       numeric::Input& output) {
    if (values_.empty()) return {numeric::Status::InvalidArgument, "input buffer not configured"};
    const short type = dbGetLinkDBFtype(&link);
    long count = static_cast<long>(values_.size());
    long status = -1;
    switch (type) {
    case DBF_DOUBLE:
        status = dbGetLink(&link, DBR_DOUBLE, values_.data(), nullptr, &count); break;
    case DBF_FLOAT:
        floats_.resize(values_.size());
        status = dbGetLink(&link, DBR_FLOAT, floats_.data(), nullptr, &count); break;
    case DBF_LONG:
        integers_.resize(values_.size());
        status = dbGetLink(&link, DBR_LONG, integers_.data(), nullptr, &count); break;
    case DBF_INT64:
        // Classic CA cannot be assumed to transport Int64 without rounding.
        if (link.type != DB_LINK) return {numeric::Status::InvalidArgument, "Int64 requires a local database link"};
        integers64_.resize(values_.size());
        status = dbGetLink(&link, DBR_INT64, integers64_.data(), nullptr, &count); break;
    default:
        return {numeric::Status::InvalidArgument, "unsupported input link type"};
    }
    if (status || count <= 0 || static_cast<unsigned long>(count) > maximum ||
        (shape == numeric::Shape::Scalar && count != 1))
        return {numeric::Status::InvalidArgument, "input link fetch, count or capacity error"};
    for (long i = 0; type != DBF_DOUBLE && i < count; ++i) {
        if (type == DBF_FLOAT) values_[i] = floats_[i];
        else if (type == DBF_LONG) values_[i] = integers_[i];
        else if (type == DBF_INT64) {
            const numeric::ElementValue source{numeric::ElementType::Int64, static_cast<std::int64_t>(integers64_[i]), 0, 0};
            numeric::ElementValue converted{};
            if (numeric::convertElement(source, numeric::ElementType::Float64, converted).status != numeric::Status::Ok)
                return {numeric::Status::InvalidArgument, "Int64 input cannot be represented exactly as Float64"};
            values_[i] = converted.real;
        }
        if (!std::isfinite(values_[i])) return {numeric::Status::InvalidArgument, "input contains a non-finite value"};
    }
    output = {values_.data(), static_cast<std::size_t>(count), values_.size(), 1, numeric::Type::Float64, shape};
    return {numeric::Status::Ok, "ok"};
}

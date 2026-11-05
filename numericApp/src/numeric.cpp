// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_version.h>
#if GSL_MAJOR_VERSION < 2 || (GSL_MAJOR_VERSION == 2 && GSL_MINOR_VERSION < 8)
#error "numeric requires GSL 2.8 or newer"
#endif
#include <atomic>
#include <cmath>
#include <mutex>
namespace numeric {
namespace {
std::once_flag initOnce;
std::atomic<bool> initialized(false);
}
void initializeGsl() {
    std::call_once(initOnce, [] {
        gsl_set_error_handler_off();
        initialized.store(true, std::memory_order_release);
    });
}
bool isGslInitialized() { return initialized.load(std::memory_order_acquire); }
Result validateArguments(const std::vector<ArgumentDescriptor>& descriptors,
                         const ArgumentValue* values, std::size_t count,
                         ArgumentValidator validator) {
    if (count > descriptors.size() || (count && !values))
        return {Status::InvalidArgument, "invalid argument count or buffer"};
    for (std::size_t i = 0; i < descriptors.size(); ++i) {
        const ArgumentDescriptor& d = descriptors[i];
        if (d.name.empty() || (d.type != ArgumentType::Floating && d.type != ArgumentType::Integer &&
            d.type != ArgumentType::Boolean && d.type != ArgumentType::Enumeration))
            return {Status::InvalidArgument, "invalid argument descriptor"};
        if ((d.hasMinimum && d.minimum.type != d.type) || (d.hasMaximum && d.maximum.type != d.type) ||
            (d.hasDefault && d.defaultValue.type != d.type))
            return {Status::InvalidArgument, "argument descriptor type mismatch"};
        if (i >= count) {
            if (d.required) return {Status::InvalidArgument, "required argument missing"};
            continue;
        }
        const ArgumentValue& v = values[i];
        if (v.type != d.type) return {Status::InvalidArgument, "argument type mismatch"};
        if (d.type == ArgumentType::Floating) {
            if (!std::isfinite(v.floating) || (d.hasMinimum && (!std::isfinite(d.minimum.floating) || v.floating < d.minimum.floating)) ||
                (d.hasMaximum && (!std::isfinite(d.maximum.floating) || v.floating > d.maximum.floating)))
                return {Status::InvalidArgument, "floating argument out of range"};
        } else {
            if ((d.type == ArgumentType::Boolean && v.integer != 0 && v.integer != 1) ||
                (d.hasMinimum && v.integer < d.minimum.integer) ||
                (d.hasMaximum && v.integer > d.maximum.integer))
                return {Status::InvalidArgument, "integer argument out of range"};
            if (d.type == ArgumentType::Enumeration) {
                bool found = false;
                for (const auto choice : d.choices) if (choice == v.integer) found = true;
                if (!found) return {Status::InvalidArgument, "unknown enumeration value"};
            }
        }
    }
    return validator ? validator(values, count) : Result{Status::Ok, "ok"};
}
Result Operation::execute(const Input* inputs, std::size_t inputCount,
                          Output* outputs, std::size_t outputCount) {
    if (!isGslInitialized()) {
        if (outputs) for (std::size_t i = 0; i < outputCount; ++i) outputs[i].count = 0;
        return {Status::NotInitialized, "GSL policy not initialized"};
    }
    return executeImpl(inputs, inputCount, outputs, outputCount);
}
bool Registry::add(const Metadata& metadata, Factory factory) {
    if (metadata.name.empty() || metadata.category.empty() || metadata.inputs.empty() ||
        metadata.outputs.empty() || metadata.parameters.empty() || metadata.lifecycle.empty() ||
        metadata.errors.empty() || !factory || find(metadata.name)) return false;
    for (const auto& d : metadata.inputs)
        if (d.type != Type::Float64 || (d.shape != Shape::Scalar && d.shape != Shape::Array)) return false;
    for (const auto& d : metadata.outputs)
        if (d.type != Type::Float64 || (d.shape != Shape::Scalar && d.shape != Shape::Array)) return false;
    entries_.push_back(Entry{metadata, factory});
    return true;
}
const Metadata* Registry::find(const std::string& name) const {
    for (const auto& entry : entries_) if (name == entry.metadata.name) return &entry.metadata;
    return nullptr;
}
std::unique_ptr<Operation> Registry::create(const std::string& name) const {
    for (const auto& entry : entries_) if (name == entry.metadata.name) return entry.factory();
    return std::unique_ptr<Operation>();
}
}

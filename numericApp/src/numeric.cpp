// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <atomic>
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
Result Operation::execute(const Input* inputs, std::size_t inputCount,
                          Output* outputs, std::size_t outputCount) {
    if (!initialized.load(std::memory_order_acquire)) {
        if (outputs && outputCount == 1) outputs[0].count = 0;
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

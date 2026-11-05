// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_version.h>
#if GSL_MAJOR_VERSION < 2 || (GSL_MAJOR_VERSION == 2 && GSL_MINOR_VERSION < 8)
#error "numeric requires GSL 2.8 or newer"
#endif
#include <atomic>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <locale>
#include <limits>
#include <mutex>
#include <sstream>
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
namespace {
bool validArgumentValue(const ArgumentDescriptor& d, const ArgumentValue& v) {
    if (v.type != d.type) return false;
    if (d.type == ArgumentType::Floating)
        return std::isfinite(v.floating) && (!d.hasMinimum || v.floating >= d.minimum.floating) &&
               (!d.hasMaximum || v.floating <= d.maximum.floating);
    if (d.type == ArgumentType::Boolean && v.integer != 0 && v.integer != 1) return false;
    if ((d.hasMinimum && v.integer < d.minimum.integer) ||
        (d.hasMaximum && v.integer > d.maximum.integer)) return false;
    if (d.type == ArgumentType::Enumeration) {
        for (const auto choice : d.choices) if (choice == v.integer) return true;
        return false;
    }
    return true;
}
}
Result validateArgumentSchema(const std::vector<ArgumentDescriptor>& descriptors) {
    bool optionalSeen = false;
    for (std::size_t i = 0; i < descriptors.size(); ++i) {
        const ArgumentDescriptor& d = descriptors[i];
        if (d.name.empty() || (d.type != ArgumentType::Floating && d.type != ArgumentType::Integer &&
            d.type != ArgumentType::Boolean && d.type != ArgumentType::Enumeration))
            return {Status::InvalidArgument, "invalid argument descriptor"};
        for (std::size_t j = 0; j < i; ++j)
            if (descriptors[j].name == d.name) return {Status::InvalidArgument, "duplicate argument name"};
        if (optionalSeen && d.required) return {Status::InvalidArgument, "required argument after optional argument"};
        if (!d.required) optionalSeen = true;
        if ((d.hasMinimum && d.minimum.type != d.type) || (d.hasMaximum && d.maximum.type != d.type) ||
            (d.hasDefault && d.defaultValue.type != d.type))
            return {Status::InvalidArgument, "argument descriptor type mismatch"};
        if (d.type == ArgumentType::Floating) {
            if ((d.hasMinimum && !std::isfinite(d.minimum.floating)) ||
                (d.hasMaximum && !std::isfinite(d.maximum.floating)) ||
                (d.hasMinimum && d.hasMaximum && d.minimum.floating > d.maximum.floating))
                return {Status::InvalidArgument, "invalid floating argument bounds"};
        } else if (d.hasMinimum && d.hasMaximum && d.minimum.integer > d.maximum.integer) {
            return {Status::InvalidArgument, "invalid integer argument bounds"};
        }
        if (d.type == ArgumentType::Enumeration && d.choices.empty())
            return {Status::InvalidArgument, "empty enumeration"};
        if (d.type != ArgumentType::Enumeration && !d.choices.empty())
            return {Status::InvalidArgument, "choices on non-enumeration"};
        for (std::size_t j = 0; j < d.choices.size(); ++j) {
            if ((d.hasMinimum && d.choices[j] < d.minimum.integer) ||
                (d.hasMaximum && d.choices[j] > d.maximum.integer))
                return {Status::InvalidArgument, "enumeration choice out of bounds"};
            for (std::size_t k = 0; k < j; ++k)
                if (d.choices[j] == d.choices[k]) return {Status::InvalidArgument, "duplicate enumeration choice"};
        }
        if (d.hasDefault && !validArgumentValue(d, d.defaultValue))
            return {Status::InvalidArgument, "invalid argument default"};
    }
    return {Status::Ok, "ok"};
}
Result validateArguments(const std::vector<ArgumentDescriptor>& descriptors,
                         const ArgumentValue* values, std::size_t count,
                         ArgumentValidator validator) {
    if (count > descriptors.size() || (count && !values))
        return {Status::InvalidArgument, "invalid argument count or buffer"};
    const Result schema = validateArgumentSchema(descriptors);
    if (schema.status != Status::Ok) return schema;
    for (std::size_t i = 0; i < descriptors.size(); ++i) {
        const ArgumentDescriptor& d = descriptors[i];
        if (i >= count) {
            if (d.required) return {Status::InvalidArgument, "required argument missing"};
            continue;
        }
        if (!validArgumentValue(d, values[i])) return {Status::InvalidArgument, "invalid argument value"};
    }
    return validator ? validator(values, count) : Result{Status::Ok, "ok"};
}
Result resolveArguments(const std::vector<ArgumentDescriptor>& descriptors,
                        const ArgumentValue* values, std::size_t count,
                        ArgumentValue* resolved, std::size_t capacity,
                        ArgumentValidator validator) {
    if (count > descriptors.size() || capacity < descriptors.size() ||
        (count && !values) || (!descriptors.empty() && !resolved))
        return {Status::InvalidArgument, "invalid argument buffers or count"};
    const Result schema = validateArgumentSchema(descriptors);
    if (schema.status != Status::Ok) return schema;
    for (std::size_t i = 0; i < descriptors.size(); ++i) {
        if (i < count) {
            if (!validArgumentValue(descriptors[i], values[i]))
                return {Status::InvalidArgument, "invalid argument value"};
        } else if (descriptors[i].required || !descriptors[i].hasDefault) {
            return {Status::InvalidArgument, "argument missing without default"};
        }
    }
    for (std::size_t i = 0; i < descriptors.size(); ++i)
        resolved[i] = i < count ? values[i] : descriptors[i].defaultValue;
    return validator ? validator(resolved, descriptors.size()) : Result{Status::Ok, "ok"};
}
Result parseArgument(ArgumentType type, const char* text, std::size_t length, ArgumentValue& value) {
    if (!text || !length || length > 38) return {Status::InvalidArgument, "invalid argument text length"};
    const std::string input(text, length);
    if (input.find('\0') != std::string::npos) return {Status::InvalidArgument, "embedded null in argument"};
    if (type == ArgumentType::Floating) {
        std::istringstream stream(input); stream.imbue(std::locale::classic());
        double parsed = 0.0;
        stream >> std::noskipws >> parsed;
        if (!stream || stream.peek() != std::char_traits<char>::eof() || !std::isfinite(parsed))
            return {Status::InvalidArgument, "invalid floating argument"};
        if (parsed == 0.0) {
            for (const char character : input) {
                if (character == 'e' || character == 'E') break;
                if (character >= '1' && character <= '9')
                    return {Status::InvalidArgument, "floating argument underflow"};
            }
        }
        value = {type, parsed, 0};
        return {Status::Ok, "ok"};
    }
    if (type == ArgumentType::Boolean && (input == "true" || input == "false")) {
        value = {type, 0.0, input == "true" ? 1 : 0};
        return {Status::Ok, "ok"};
    }
    if (type != ArgumentType::Integer && type != ArgumentType::Boolean && type != ArgumentType::Enumeration)
        return {Status::InvalidArgument, "unknown argument type"};
    std::size_t pos = (input[0] == '-' || input[0] == '+') ? 1 : 0;
    if (pos == input.size()) return {Status::InvalidArgument, "invalid integer argument"};
    for (; pos < input.size(); ++pos)
        if (input[pos] < '0' || input[pos] > '9') return {Status::InvalidArgument, "invalid integer argument"};
    errno = 0; char* end = nullptr;
    const long long parsed = std::strtoll(input.c_str(), &end, 10);
    if (errno == ERANGE || end != input.c_str() + input.size() ||
        parsed < std::numeric_limits<std::int64_t>::min() || parsed > std::numeric_limits<std::int64_t>::max() ||
        (type == ArgumentType::Boolean && parsed != 0 && parsed != 1))
        return {Status::InvalidArgument, "integer argument out of range"};
    value = {type, 0.0, static_cast<std::int64_t>(parsed)};
    return {Status::Ok, "ok"};
}
namespace {
bool exactlyRepresentable(std::int64_t value) {
    std::uint64_t magnitude = value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1 :
                                          static_cast<std::uint64_t>(value);
    while (magnitude > UINT64_C(9007199254740992)) {
        if (magnitude & 1u) return false;
        magnitude >>= 1;
    }
    return true;
}
}
Result setLegacyArguments(const Metadata& metadata, const ArgumentValue* values,
                          std::size_t count, Input* inputs, std::size_t inputCount,
                          double* slots, std::size_t slotCount) {
    if (!metadata.argumentSchemaComplete || count != metadata.arguments.size() || slotCount < count ||
        (count && (!inputs || !slots)))
        return {Status::InvalidArgument, "invalid argument buffers or count"};
    const Result validated = validateArguments(metadata.arguments, values, count, metadata.argumentValidator);
    if (validated.status != Status::Ok) return validated;
    for (std::size_t i = 0; i < count; ++i) {
        const std::size_t index = metadata.arguments[i].inputIndex;
        if (index >= inputCount || index >= metadata.inputs.size() ||
            metadata.inputs[index].type != Type::Float64 || metadata.inputs[index].shape != Shape::Scalar ||
            (values[i].type != ArgumentType::Floating && !exactlyRepresentable(values[i].integer)))
            return {Status::InvalidArgument, "invalid mapping or inexact integer conversion"};
        for (std::size_t j = 0; j < i; ++j)
            if (metadata.arguments[j].inputIndex == index)
                return {Status::InvalidArgument, "duplicate argument mapping"};
    }
    for (std::size_t i = 0; i < count; ++i) {
        slots[i] = values[i].type == ArgumentType::Floating ? values[i].floating : static_cast<double>(values[i].integer);
        inputs[metadata.arguments[i].inputIndex] = {&slots[i], 1, 1, 1, Type::Float64, Shape::Scalar};
    }
    return {Status::Ok, "ok"};
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
        metadata.errors.empty() || !factory || find(metadata.name) ||
        validateArgumentSchema(metadata.arguments).status != Status::Ok) return false;
    for (const auto& d : metadata.inputs)
        if (d.type != Type::Float64 || (d.shape != Shape::Scalar && d.shape != Shape::Array)) return false;
    for (const auto& d : metadata.outputs)
        if (d.type != Type::Float64 || (d.shape != Shape::Scalar && d.shape != Shape::Array)) return false;
    for (std::size_t i = 0; i < metadata.arguments.size(); ++i) {
        const std::size_t index = metadata.arguments[i].inputIndex;
        if (index == static_cast<std::size_t>(-1)) continue;
        if (index >= metadata.inputs.size() || metadata.inputs[index].shape != Shape::Scalar ||
            metadata.inputs[index].type != Type::Float64) return false;
        for (std::size_t j = 0; j < i; ++j)
            if (metadata.arguments[j].inputIndex == index) return false;
    }
    entries_.push_back(Entry{metadata, factory});
    return true;
}
bool Registry::addWithoutArguments(Metadata metadata, Factory factory) {
    if (!metadata.arguments.empty() || metadata.argumentValidator) return false;
    metadata.argumentSchemaComplete = true;
    return add(metadata, factory);
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

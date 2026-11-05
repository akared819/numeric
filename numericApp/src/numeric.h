// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_H
#define NUMERIC_H
#include "numericExport.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace numeric {
enum class Type { Float64 };
enum class Shape { Scalar, Array };
struct Descriptor { Type type; Shape shape; };
struct Input { const double* data; std::size_t count; std::size_t capacity; std::size_t stride; Type type; Shape shape; };
struct Output { double* data; std::size_t capacity; std::size_t count; Type type; Shape shape; };
enum class Status { Ok, InvalidArgument, NumericalError, NotInitialized };
struct Result { Status status; const char* message; };
// Values are tagged so integer arguments never pass through floating point.
enum class ArgumentType { Floating, Integer, Boolean, Enumeration };
struct ArgumentValue {
    ArgumentType type;
    double floating;
    std::int64_t integer;
};
struct ArgumentDescriptor {
    std::string name;
    ArgumentType type;
    bool required;
    bool hasDefault;
    ArgumentValue defaultValue;
    bool hasMinimum;
    bool hasMaximum;
    ArgumentValue minimum;
    ArgumentValue maximum;
    std::vector<std::int64_t> choices;
};
using ArgumentValidator = Result (*)(const ArgumentValue*, std::size_t);
// Validate a complete, ordered argument list; defaults are not applied here.
NUMERIC_API Result validateArguments(const std::vector<ArgumentDescriptor>& descriptors,
                                     const ArgumentValue* values, std::size_t count,
                                     ArgumentValidator validator = nullptr);
struct Metadata {
    std::string name; std::string category;
    std::vector<Descriptor> inputs, outputs;
    std::string parameters; bool stateful;
    std::string lifecycle; std::string errors;
    std::vector<ArgumentDescriptor> arguments;
    ArgumentValidator argumentValidator = nullptr;
};
class NUMERIC_API Operation {
public:
    virtual ~Operation() {}
    Result execute(const Input* inputs, std::size_t inputCount,
                   Output* outputs, std::size_t outputCount);
    virtual void reset() = 0;
protected:
    virtual Result executeImpl(const Input* inputs, std::size_t inputCount,
                               Output* outputs, std::size_t outputCount) = 0;
};
using Factory = std::unique_ptr<Operation> (*)();
class NUMERIC_API Registry {
public:
    bool add(const Metadata& metadata, Factory factory);
    const Metadata* find(const std::string& name) const;
    std::unique_ptr<Operation> create(const std::string& name) const;
private:
    struct Entry { Metadata metadata; Factory factory; };
    std::vector<Entry> entries_;
};
// Call once during single-threaded process startup, before any other GSL calls.
NUMERIC_API void initializeGsl();
NUMERIC_API bool isGslInitialized();
NUMERIC_API void registerBuiltins(Registry& registry);
}
#endif

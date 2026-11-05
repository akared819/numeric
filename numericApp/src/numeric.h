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
    // Index of the legacy scalar input consumed by this parameter, or -1 if
    // it is not mapped to the legacy execution interface.
    std::size_t inputIndex = static_cast<std::size_t>(-1);
};
using ArgumentValidator = Result (*)(const ArgumentValue*, std::size_t);
// Check descriptor consistency, including bounds, defaults, and unique names.
NUMERIC_API Result validateArgumentSchema(const std::vector<ArgumentDescriptor>& descriptors);
// Validate a complete, ordered argument list; defaults are not applied here.
NUMERIC_API Result validateArguments(const std::vector<ArgumentDescriptor>& descriptors,
                                     const ArgumentValue* values, std::size_t count,
                                     ArgumentValidator validator = nullptr);
// Fill caller-owned output with supplied values and trailing defaults. On
// failure output contents are unspecified. Input and output must not overlap.
NUMERIC_API Result resolveArguments(const std::vector<ArgumentDescriptor>& descriptors,
                                    const ArgumentValue* values, std::size_t count,
                                    ArgumentValue* resolved, std::size_t capacity,
                                    ArgumentValidator validator = nullptr);
// Parse an ASCII argument with no whitespace; integer input remains exact.
NUMERIC_API Result parseArgument(ArgumentType type, const char* text, std::size_t length,
                                 ArgumentValue& value);
struct Metadata {
    std::string name; std::string category;
    std::vector<Descriptor> inputs, outputs;
    std::string parameters; bool stateful;
    std::string lifecycle; std::string errors;
    std::vector<ArgumentDescriptor> arguments;
    ArgumentValidator argumentValidator = nullptr;
    // True means the descriptor list is complete, including an empty list.
    bool argumentSchemaComplete = false;
};
// Build mapped scalar inputs using caller-owned slots. Reject integers that
// cannot be represented exactly by the legacy double-based interface.
NUMERIC_API Result setLegacyArguments(const Metadata& metadata, const ArgumentValue* values,
                                      std::size_t count, Input* inputs, std::size_t inputCount,
                                      double* slots, std::size_t slotCount);
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
    bool addWithoutArguments(Metadata metadata, Factory factory);
    const Metadata* find(const std::string& name) const;
    std::unique_ptr<Operation> create(const std::string& name) const;
private:
    struct Entry { Metadata metadata; Factory factory; };
    std::vector<Entry> entries_;
};
// Call once during single-threaded process startup, before any other GSL calls.
NUMERIC_API void initializeGsl();
NUMERIC_API bool isGslInitialized();
// Synchronous native callback. Caller owns context and must keep it alive for the call.
using ScalarCallback = double (*)(double x, void* context);
// Output values are unchanged on error. The error value is an estimate, not a bound.
NUMERIC_API Result differentiateCentral(ScalarCallback callback, void* context, double x, double step,
                                         double& value, double& absoluteError);
// Adaptive integral over a finite increasing interval, with a bounded subdivision count.
NUMERIC_API Result integrateAdaptive(ScalarCallback callback, void* context, double lower, double upper,
                                     double absoluteTolerance, double relativeTolerance, std::size_t maxSubintervals,
                                     double& value, double& absoluteError);
NUMERIC_API void registerBuiltins(Registry& registry);
}
#endif

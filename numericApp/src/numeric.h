#ifndef NUMERIC_H
#define NUMERIC_H
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace numeric {
enum class Type { Float64 };
enum class Shape { Scalar, Array };
struct Descriptor { Type type; Shape shape; };
struct Input { const double* data; std::size_t count; std::size_t capacity; std::size_t stride; Type type; Shape shape; };
struct Output { double* data; std::size_t capacity; std::size_t count; Type type; Shape shape; };
enum class Status { Ok, InvalidArgument, NumericalError };
struct Result { Status status; const char* message; };
struct Metadata {
    const char* name; const char* category;
    std::vector<Descriptor> inputs, outputs;
    const char* parameters; bool stateful;
    const char* lifecycle; const char* errors;
};
class Operation {
public:
    virtual ~Operation() {}
    virtual Result execute(const Input* inputs, std::size_t inputCount,
                           Output* outputs, std::size_t outputCount) = 0;
    virtual void reset() = 0;
};
using Factory = std::unique_ptr<Operation> (*)();
class Registry {
public:
    bool add(const Metadata& metadata, Factory factory);
    const Metadata* find(const std::string& name) const;
    std::unique_ptr<Operation> create(const std::string& name) const;
private:
    struct Entry { Metadata metadata; Factory factory; };
    std::vector<Entry> entries_;
};
// Call once during single-threaded process startup, before any other GSL calls.
void initializeGsl();
void registerBuiltins(Registry& registry);
}
#endif

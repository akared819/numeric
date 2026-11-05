// SPDX-License-Identifier: GPL-3.0-or-later
// Executable examples for the native core reference.
#include "numeric.h"
#include "numericData.h"
#include "numericTypedMean.h"
#include "numericAck.h"
#include <cmath>
#include <cstdio>
#include <memory>
using namespace numeric;

class Echo : public Operation {
public:
    void reset() override {}
protected:
    Result executeImpl(const Input* in, std::size_t ni, Output* out, std::size_t no) override {
        if (out) for (std::size_t i = 0; i < no; ++i) out[i].count = 0;
        if (!in || !out || ni != 1 || no != 1 || !in[0].data || in[0].count != 1 ||
            in[0].capacity < 1 || in[0].stride != 1 || in[0].type != Type::Float64 ||
            in[0].shape != Shape::Scalar || !std::isfinite(in[0].data[0]) ||
            !out[0].data || out[0].capacity < 1 || out[0].type != Type::Float64 ||
            out[0].shape != Shape::Scalar) return {Status::InvalidArgument, "invalid input"};
        out[0].data[0] = in[0].data[0]; out[0].count = 1;
        return {Status::Ok, "ok"};
    }
};
std::unique_ptr<Operation> makeEcho() { return std::unique_ptr<Operation>(new Echo); }
#define REQUIRE(expr) do { if (!(expr)) { std::fprintf(stderr, "Failed: %s\n", #expr); return 1; } } while (0)
int main() {
    initializeGsl();
    REQUIRE(isGslInitialized());
    Registry registry; registerBuiltins(registry);
    const Metadata* meta = registry.find("pow_int");
    REQUIRE(meta && validateArgumentSchema(meta->arguments).status == Status::Ok);
    ArgumentValue parsed{}, resolved[1]{};
    REQUIRE(parseArgument(ArgumentType::Integer, "3", 1, parsed).status == Status::Ok);
    REQUIRE(validateArguments(meta->arguments, &parsed, 1).status == Status::Ok);
    REQUIRE(resolveArguments(meta->arguments, nullptr, 0, resolved, 1).status == Status::Ok);
    REQUIRE(resolved[0].integer == 2);
    double base = 2, slots[1] = {}, powerValue = 0;
    Input powerInputs[2] = {{&base, 1, 1, 1, Type::Float64, Shape::Scalar}, {}};
    REQUIRE(setLegacyArguments(*meta, &parsed, 1, powerInputs, 2, slots, 1).status == Status::Ok);
    auto power = registry.create("pow_int"); REQUIRE(power);
    Output powerOutput{&powerValue, 1, 0, Type::Float64, Shape::Scalar};
    REQUIRE(power->execute(powerInputs, 2, &powerOutput, 1).status == Status::Ok && powerValue == 8);
    power->reset();
    Metadata echo{"example_echo", "example", {{Type::Float64, Shape::Scalar}},
                  {{Type::Float64, Shape::Scalar}}, "none", false, "per instance", "invalid input"};
    REQUIRE(registry.addWithoutArguments(echo, makeEcho));
    echo.name = "example_echo_2"; echo.argumentSchemaComplete = true;
    REQUIRE(registry.add(echo, makeEcho));
    REQUIRE(registry.find("example_echo") && registry.create("example_echo_2"));

    const std::size_t shape[] = {3}, strides[] = {sizeof(double)}, index[] = {1};
    double data[] = {1, 2, 3};
    ArrayView view{data, sizeof(data), ElementType::Float64, 1, shape, strides};
    MutableArrayView writable{data, sizeof(data), ElementType::Float64, 1, shape, strides};
    REQUIRE(validateArray(view).status == Status::Ok && validateWritableArray(writable).status == Status::Ok);
    ElementValue element{}, converted{};
    REQUIRE(readElement(view, index, 1, element).status == Status::Ok && element.real == 2);
    REQUIRE(convertElement(element, ElementType::Int32, converted).status == Status::Ok && converted.integer == 2);
    element.real = 4;
    REQUIRE(writeElement(writable, index, 1, element).status == Status::Ok && data[1] == 4);
    TypedMean mean(3); REQUIRE(mean.reserve().status == Status::Ok);
    double meanValue = 0;
    REQUIRE(mean.execute(view, meanValue).status == Status::Ok && std::fabs(meanValue - 8.0/3) < 1e-12);
    mean.reset();
    TypedComplexMean complexMean(3); REQUIRE(complexMean.reserve().status == Status::Ok);
    ElementValue complexValue{};
    REQUIRE(complexMean.execute(view, complexValue).status == Status::Ok && complexValue.imaginary == 0);
    complexMean.reset();

    UniformRandomStream random; REQUIRE(random.initialize(123).status == Status::Ok);
    double first[2] = {}, repeated[2] = {};
    REQUIRE(random.fill(first, 2, 2).status == Status::Ok);
    REQUIRE(random.reset().status == Status::Ok && random.fill(repeated, 2, 2).status == Status::Ok);
    REQUIRE(first[0] == repeated[0] && first[1] == repeated[1]);
    auto stream = AcknowledgedStream::create(); REQUIRE(stream);
    Event event{1, 0, EventKind::Sample, 2}; Acknowledgment ack{};
    REQUIRE(stream->submit(event, ack).status == Status::Ok && ack.statistics[0] == 2 && ack.statistics[6] == 1);
    REQUIRE(stream->submit(event, ack).status == Status::Ok && ack.replayed);
    std::puts("Native core reference examples passed");
    return 0;
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include "numericAck.h"
#include <gsl/gsl_errno.h>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <limits>
#include <thread>
#include <vector>
using namespace numeric;
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #x); return 1; } } while (0)
namespace {
int destroyed = 0;
class Lifetime : public Operation {
public:
    ~Lifetime() override { ++destroyed; }
    void reset() override {}
protected:
    Result executeImpl(const Input*, std::size_t, Output*, std::size_t) override { return {Status::Ok, "ok"}; }
};
std::unique_ptr<Operation> makeLifetime() { return std::unique_ptr<Operation>(new Lifetime); }
}
int main() {
    Registry r; registerBuiltins(r);
    auto op = r.create("mean"); CHECK(op && r.find("mean") && !r.find("missing") && !r.create("missing"));
    double data[] = {1, 99, 3, 99, 5}; double result = -1;
    Input in{data, 3, 5, 2, Type::Float64, Shape::Array};
    Output out{&result, 1, 7, Type::Float64, Shape::Scalar};
    CHECK(op->execute(&in, 1, &out, 1).status == Status::NotInitialized && out.count == 0 && result == -1);
    Lifetime beforeInit; Output uninitializedOut[2] = {
        {&result, 1, 5, Type::Float64, Shape::Scalar}, {&result, 1, 5, Type::Float64, Shape::Scalar}};
    CHECK(beforeInit.execute(nullptr, 0, uninitializedOut, 2).status == Status::NotInitialized &&
          uninitializedOut[0].count == 0 && uninitializedOut[1].count == 0);
    CHECK(r.find("running_mean") && r.find("running_mean")->stateful && !r.create("running_mean"));
    CHECK(!AcknowledgedStream::create());
    initializeGsl(); initializeGsl(); gsl_error("test", __FILE__, __LINE__, GSL_EDOM);
    CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok && out.count == 1 && std::fabs(result - 3) < 1e-12);
    auto minmax = r.create("minmax");
    double extrema[] = {99, 99};
    Output extremaOut[] = {{&extrema[0], 1, 0, Type::Float64, Shape::Scalar},
                           {&extrema[1], 1, 0, Type::Float64, Shape::Scalar}};
    CHECK(minmax && r.find("minmax") && r.find("minmax")->outputs.size() == 2);
    CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::Ok && extrema[0] == 1 && extrema[1] == 5);
    double mixed[] = {-4, 8, -4, 8}; Input mixedIn{mixed, 4, 4, 1, Type::Float64, Shape::Array};
    CHECK(minmax->execute(&mixedIn, 1, extremaOut, 2).status == Status::Ok && extrema[0] == -4 && extrema[1] == 8);
    in.count = 1; CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::Ok && extrema[0] == 1 && extrema[1] == 1);
    in.count = 0; CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument && extremaOut[0].count == 0 && extremaOut[1].count == 0);
    in.count = 3; in.capacity = 4; CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument);
    in.capacity = 5; in.stride = 0; CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument);
    in.stride = std::numeric_limits<std::size_t>::max(); CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument);
    in.stride = 2; extremaOut[1].data = extremaOut[0].data;
    CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument);
    extremaOut[1].data = &extrema[1]; extremaOut[1].capacity = 0;
    CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument);
    extremaOut[1].capacity = 1; data[2] = std::numeric_limits<double>::quiet_NaN();
    CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument && extrema[0] == 1 && extrema[1] == 1);
    data[2] = std::numeric_limits<double>::infinity();
    CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::InvalidArgument);
    data[2] = 3; CHECK(minmax->execute(&in, 1, extremaOut, 2).status == Status::Ok && extrema[0] == 1 && extrema[1] == 5);
    auto covariance = r.create("covariance");
    CHECK(covariance && r.find("covariance") && r.find("covariance")->inputs.size() == 2);
    double ydata[] = {2, 4, 6};
    Input pair[] = {in, {ydata, 3, 3, 1, Type::Float64, Shape::Array}};
    double covResult = 42;
    Output covOut{&covResult, 1, 0, Type::Float64, Shape::Scalar};
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::Ok && covOut.count == 1 && std::fabs(covResult - 4) < 1e-12);
    double interleaved[] = {1, 2, 3, 4, 5, 6};
    Input packed[] = {{interleaved, 3, 6, 2, Type::Float64, Shape::Array},
                      {interleaved + 1, 3, 5, 2, Type::Float64, Shape::Array}};
    CHECK(covariance->execute(packed, 2, &covOut, 1).status == Status::Ok && covResult == 4);
    packed[1].capacity = 4;
    CHECK(covariance->execute(packed, 2, &covOut, 1).status == Status::InvalidArgument && covResult == 4);
    ydata[0] = 6; ydata[2] = 2;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::Ok && covResult == -4);
    ydata[0] = ydata[1] = ydata[2] = 2;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::Ok && covResult == 0);
    ydata[0] = 2; ydata[1] = 4; ydata[2] = 6;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::Ok && covResult == 4);
    pair[1].count = 2; CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument && covOut.count == 0 && covResult == 4);
    pair[0].count = pair[1].count = 1; CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[0].count = pair[1].count = 0; CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[0].count = pair[1].count = 3; pair[1].capacity = 2;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[1].capacity = 3; pair[0].stride = 0;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[0].stride = std::numeric_limits<std::size_t>::max();
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[0].stride = 2; ydata[1] = std::numeric_limits<double>::quiet_NaN();
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument && covResult == 4);
    ydata[1] = std::numeric_limits<double>::infinity();
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    ydata[1] = 4; covOut.capacity = 0;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    covOut.capacity = 1; covOut.shape = Shape::Array;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    covOut.shape = Shape::Scalar; pair[1].shape = Shape::Scalar;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[1].shape = Shape::Array; pair[1].data = nullptr;
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[1].data = ydata;
    CHECK(covariance->execute(pair, 1, &covOut, 1).status == Status::InvalidArgument);
    CHECK(covariance->execute(pair, 2, &covOut, 1).status == Status::Ok && covResult == 4);
    double huge[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
    Input hugePair[] = {{huge, 2, 2, 1, Type::Float64, Shape::Array}, {huge, 2, 2, 1, Type::Float64, Shape::Array}};
    CHECK(covariance->execute(hugePair, 2, &covOut, 1).status == Status::NumericalError && covOut.count == 0 && covResult == 4);
    auto correlation = r.create("correlation");
    CHECK(correlation && r.find("correlation") && r.find("correlation")->inputs.size() == 2);
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::Ok && std::fabs(covResult - 1) < 1e-12);
    packed[1].capacity = 5;
    CHECK(correlation->execute(packed, 2, &covOut, 1).status == Status::Ok && std::fabs(covResult - 1) < 1e-12);
    ydata[0] = 6; ydata[2] = 2;
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::Ok && std::fabs(covResult + 1) < 1e-12);
    ydata[0] = ydata[1] = ydata[2] = 2;
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument && covOut.count == 0 && std::fabs(covResult + 1) < 1e-12);
    double constantX[] = {3, 3, 3};
    Input constantPair[] = {{constantX, 3, 3, 1, Type::Float64, Shape::Array},
                            {data, 3, 5, 2, Type::Float64, Shape::Array}};
    CHECK(correlation->execute(constantPair, 2, &covOut, 1).status == Status::InvalidArgument);
    double extremeCorrelation[] = {1e200, -1e200};
    Input extremePair[] = {{extremeCorrelation, 2, 2, 1, Type::Float64, Shape::Array},
                           {extremeCorrelation, 2, 2, 1, Type::Float64, Shape::Array}};
    CHECK(correlation->execute(extremePair, 2, &covOut, 1).status == Status::NumericalError && covOut.count == 0);
    double tinyCorrelation[] = {1e-200, -1e-200};
    Input tinyPair[] = {{tinyCorrelation, 2, 2, 1, Type::Float64, Shape::Array},
                        {tinyCorrelation, 2, 2, 1, Type::Float64, Shape::Array}};
    CHECK(correlation->execute(tinyPair, 2, &covOut, 1).status == Status::NumericalError && covOut.count == 0);
    Input unsafeSecond[] = {{data, 2, 5, 2, Type::Float64, Shape::Array},
                            {extremeCorrelation, 2, 2, 1, Type::Float64, Shape::Array}};
    CHECK(correlation->execute(unsafeSecond, 2, &covOut, 1).status == Status::NumericalError);
    unsafeSecond[1].data = tinyCorrelation;
    CHECK(correlation->execute(unsafeSecond, 2, &covOut, 1).status == Status::NumericalError);
    double scaledCorrelation[] = {1e150, -1e150};
    Input scaledPair[] = {{scaledCorrelation, 2, 2, 1, Type::Float64, Shape::Array},
                          {scaledCorrelation, 2, 2, 1, Type::Float64, Shape::Array}};
    CHECK(correlation->execute(scaledPair, 2, &covOut, 1).status == Status::Ok && std::fabs(covResult - 1) < 1e-12);
    scaledCorrelation[0] = 1e-150; scaledCorrelation[1] = -1e-150;
    CHECK(correlation->execute(scaledPair, 2, &covOut, 1).status == Status::Ok && std::fabs(covResult - 1) < 1e-12);
    ydata[0] = 2; ydata[1] = 4; ydata[2] = 6; pair[1].count = 2;
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[1].count = 3; pair[0].count = pair[1].count = 1;
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[0].count = pair[1].count = 0;
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    pair[0].count = pair[1].count = 3; ydata[1] = std::numeric_limits<double>::infinity();
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    ydata[1] = std::numeric_limits<double>::quiet_NaN();
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::InvalidArgument);
    ydata[1] = 4;
    CHECK(correlation->execute(pair, 2, &covOut, 1).status == Status::Ok && std::fabs(covResult - 1) < 1e-12);
    auto variance = r.create("variance");
    auto popVar = r.create("population_variance"); auto popSd = r.create("population_stddev");
    CHECK(popVar && popSd);
    CHECK(popVar->execute(&in, 1, &out, 1).status == Status::Ok && std::fabs(result - 8.0 / 3.0) < 1e-12);
    CHECK(popSd->execute(&in, 1, &out, 1).status == Status::Ok && std::fabs(result - std::sqrt(8.0 / 3.0)) < 1e-12);
    in.count = 1;
    CHECK(popVar->execute(&in, 1, &out, 1).status == Status::Ok && result == 0);
    CHECK(popSd->execute(&in, 1, &out, 1).status == Status::Ok && result == 0);
    in.count = 0; CHECK(popVar->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    CHECK(popSd->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.count = 3;
    auto sd = r.create("stddev"); CHECK(sd && r.find("stddev"));
    CHECK(sd->execute(&in, 1, &out, 1).status == Status::Ok && std::fabs(result - 2) < 1e-12);
    in.count = 1; result = 42;
    CHECK(sd->execute(&in, 1, &out, 1).status == Status::InvalidArgument && out.count == 0 && result == 42);
    in.count = 3;
    CHECK(variance && r.find("variance") && !r.find("variance")->stateful);
    CHECK(variance->execute(&in, 1, &out, 1).status == Status::Ok && out.count == 1 && std::fabs(result - 4) < 1e-12);
    in.count = 1; result = 42;
    CHECK(variance->execute(&in, 1, &out, 1).status == Status::InvalidArgument && out.count == 0 && result == 42);
    in.count = 0; CHECK(variance->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.count = 3; in.capacity = 4; CHECK(variance->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.capacity = 5; data[2] = std::numeric_limits<double>::infinity();
    CHECK(variance->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = std::numeric_limits<double>::quiet_NaN();
    CHECK(variance->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    CHECK(sd->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    CHECK(popVar->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    CHECK(popSd->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = 3; CHECK(variance->execute(&in, 1, &out, 1).status == Status::Ok && result == 4);
    CHECK(sd->execute(&in, 1, &out, 1).status == Status::Ok && result == 2);
    double identical[] = {7, 7}; Input same{identical, 2, 2, 1, Type::Float64, Shape::Array};
    CHECK(variance->execute(&same, 1, &out, 1).status == Status::Ok && result == 0);
    double large[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
    Input overflow{large, 2, 2, 1, Type::Float64, Shape::Array}; result = 42;
    CHECK(variance->execute(&overflow, 1, &out, 1).status == Status::NumericalError && out.count == 0 && result == 42);
    in.count = 1; CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok && result == 1);
    result = 42; in.count = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument && out.count == 0 && result == 42);
    in.count = 3; in.capacity = 4; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument && result == 42);
    in.capacity = 5; in.stride = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.stride = std::numeric_limits<std::size_t>::max(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.stride = 2; out.capacity = 0; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    out.capacity = 1; in.shape = Shape::Scalar; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.shape = Shape::Array; in.data = nullptr; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    in.data = data; out.data = nullptr; CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument); out.data = &result;
    data[2] = std::numeric_limits<double>::quiet_NaN(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = std::numeric_limits<double>::infinity(); CHECK(op->execute(&in, 1, &out, 1).status == Status::InvalidArgument);
    data[2] = 3; op->reset(); CHECK(op->execute(&in, 1, &out, 1).status == Status::Ok);
    CHECK(op->execute(nullptr, 0, &out, 1).status == Status::InvalidArgument);
    double extreme[] = {std::numeric_limits<double>::max(), -std::numeric_limits<double>::max()};
    Input bad{extreme, 2, 2, 1, Type::Float64, Shape::Array}; result = 42;
    Result extremeResult = op->execute(&bad, 1, &out, 1);
    CHECK((extremeResult.status == Status::NumericalError && out.count == 0 && result == 42) ||
          (extremeResult.status == Status::Ok && out.count == 1 && result == 0));
    Metadata meta = *r.find("mean"); meta.name = "lifetime";
    CHECK(r.add(meta, &makeLifetime)); meta.name = "changed"; meta.category.clear();
    CHECK(r.find("lifetime") && r.find("lifetime")->category == "statistics");
    CHECK(!r.add(*r.find("lifetime"), &makeLifetime));
    CHECK(!r.add(meta, &makeLifetime));
    meta.category = "test"; meta.name = "bad"; meta.inputs[0].shape = static_cast<Shape>(99);
    CHECK(!r.add(meta, &makeLifetime));
    { auto owned = r.create("lifetime"); CHECK(owned && destroyed == 0); }
    CHECK(destroyed == 1);
    auto running = r.create("running_mean"); auto other = r.create("running_mean"); CHECK(running && other);
    auto stats = r.create("running_stats"); auto stats2 = r.create("running_stats");
    CHECK(stats && stats2 && r.find("running_stats")->stateful);
    double statValues[7] = {}; Output statOut[7];
    for (int i = 0; i < 7; ++i) statOut[i] = {&statValues[i], 1, 0, Type::Float64, Shape::Scalar};
    double statSample = 1; Input statInput{&statSample, 1, 1, 1, Type::Float64, Shape::Scalar};
    CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::Ok);
    CHECK(statValues[0] == 1 && std::isnan(statValues[1]) && std::isnan(statValues[2]) && statValues[3] == 1 && statValues[4] == 1 && statValues[5] == 1 && statValues[6] == 1);
    statSample = 3; CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::Ok);
    CHECK(statValues[0] == 2 && statValues[1] == 2 && std::fabs(statValues[2] - std::sqrt(2.0)) < 1e-12 && std::fabs(statValues[3] - std::sqrt(5.0)) < 1e-12 && statValues[4] == 1 && statValues[5] == 3 && statValues[6] == 2);
    CHECK(stats2->execute(&statInput, 1, statOut, 7).status == Status::Ok && statValues[6] == 1);
    statSample = std::numeric_limits<double>::infinity(); CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::InvalidArgument && statOut[0].count == 0);
    statSample = 5; CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::Ok && statValues[6] == 3 && statValues[0] == 3 && statValues[1] == 4 && statValues[4] == 1 && statValues[5] == 5);
    statOut[1].data = statOut[0].data; CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::InvalidArgument);
    statOut[1].data = &statValues[1]; stats->reset();
    CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::Ok && statValues[6] == 1 && statValues[0] == 5);
    stats->reset(); statSample = std::numeric_limits<double>::max();
    CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::Ok);
    statSample = -std::numeric_limits<double>::max();
    CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::NumericalError && statOut[0].count == 0);
    statSample = 1; CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::NumericalError);
    stats->reset(); CHECK(stats->execute(&statInput, 1, statOut, 7).status == Status::Ok && statValues[6] == 1);
    auto ack = AcknowledgedStream::create(); auto ackOther = AcknowledgedStream::create();
    CHECK(ack && ackOther);
    Acknowledgment response{}; Event event{11, 0, EventKind::Sample, 1};
    CHECK(ackOther->submit(Event{0, 0, EventKind::Sample, 1}, response).status == Status::InvalidArgument);
    CHECK(ackOther->submit(Event{12, 1, EventKind::Sample, 1}, response).status == Status::InvalidArgument);
    CHECK(ackOther->submit(Event{12, 0, static_cast<EventKind>(99), 1}, response).status == Status::InvalidArgument);
    CHECK(ack->submit(event, response).status == Status::Ok && !response.replayed && response.statistics[0] == 1 && response.statistics[6] == 1);
    CHECK(ack->submit(event, response).status == Status::Ok && response.replayed && response.statistics[6] == 1);
    event.value = 2; CHECK(ack->submit(event, response).status == Status::InvalidArgument);
    event = {11, 2, EventKind::Sample, 5}; CHECK(ack->submit(event, response).status == Status::InvalidArgument);
    event = {12, 1, EventKind::Sample, 5}; CHECK(ack->submit(event, response).status == Status::InvalidArgument);
    event = {11, 1, EventKind::Sample, std::numeric_limits<double>::quiet_NaN()}; CHECK(ack->submit(event, response).status == Status::InvalidArgument);
    event.value = 3; CHECK(ack->submit(event, response).status == Status::Ok && response.statistics[0] == 2 && response.statistics[6] == 2);
    CHECK(ackOther->submit(Event{12, 0, EventKind::Sample, 3}, response).status == Status::Ok && response.statistics[6] == 1);
    event = {11, 2, EventKind::Reset, 1}; CHECK(ack->submit(event, response).status == Status::InvalidArgument);
    event.value = 0; CHECK(ack->submit(event, response).status == Status::Ok && response.statistics[6] == 0 && std::isnan(response.statistics[0]));
    CHECK(ack->submit(event, response).status == Status::Ok && response.replayed && response.statistics[6] == 0);
    event = {11, 3, EventKind::Sample, 5}; CHECK(ack->submit(event, response).status == Status::Ok && response.statistics[0] == 5 && response.statistics[6] == 1);
    event = {11, 4, EventKind::Sample, std::numeric_limits<double>::max()};
    CHECK(ack->submit(event, response).status == Status::NumericalError);
    event = {11, 4, EventKind::Reset, 0};
    CHECK(ack->submit(event, response).status == Status::Ok && response.statistics[6] == 0);
    double sample = 1, avg = -1, count = -1;
    Input sampleIn{&sample, 1, 1, 1, Type::Float64, Shape::Scalar};
    Output runningOut[] = {{&avg, 1, 0, Type::Float64, Shape::Scalar},
                           {&count, 1, 0, Type::Float64, Shape::Scalar}};
    CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && avg == 1 && count == 1);
    sample = 3; CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && avg == 2 && count == 2);
    CHECK(other->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && avg == 3 && count == 1);
    sample = std::numeric_limits<double>::quiet_NaN();
    CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::InvalidArgument && runningOut[0].count == 0 && runningOut[1].count == 0);
    sample = 5; CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && avg == 3 && count == 3);
    runningOut[1].data = &avg;
    CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::InvalidArgument);
    runningOut[1].data = &count; running->reset();
    CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && avg == 5 && count == 1);
    CHECK(other->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && avg == 4 && count == 2);
    running->reset(); sample = std::numeric_limits<double>::max();
    CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok);
    sample = -std::numeric_limits<double>::max(); avg = 42; count = 42;
    CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::NumericalError && avg == 42 && count == 42);
    sample = 1; CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::NumericalError);
    running->reset(); CHECK(running->execute(&sampleIn, 1, runningOut, 2).status == Status::Ok && count == 1 && avg == 1);
    std::atomic<bool> good(true); std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) threads.emplace_back([&r, &good] {
        auto instance = r.create("mean"); double values[] = {1, 2, 3}, y = 0;
        Input x{values, 3, 3, 1, Type::Float64, Shape::Array}; Output o{&y, 1, 0, Type::Float64, Shape::Scalar};
        for (int i = 0; i < 1000; ++i)
            if (instance->execute(&x, 1, &o, 1).status != Status::Ok || y != 2) good = false;
    });
    for (auto& thread : threads) thread.join(); CHECK(good.load());
    std::puts("numericTest: all checks passed"); return 0;
}

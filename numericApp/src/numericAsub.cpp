// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <aSubRecord.h>
#include <alarm.h>
#include <dbFldTypes.h>
#include <dbLink.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <recGbl.h>
#include <registryFunction.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <cstdio>
#include <cmath>
#include <limits>
#include <map>

namespace {
std::once_flag setupOnce;
std::unique_ptr<numeric::Operation> mean;
std::unique_ptr<numeric::Operation> variance;
std::unique_ptr<numeric::Operation> stddev;
std::unique_ptr<numeric::Operation> populationVariance, populationStddev;
std::unique_ptr<numeric::Operation> minmax;
std::unique_ptr<numeric::Operation> covariance;
std::unique_ptr<numeric::Operation> correlation;
std::unique_ptr<numeric::Operation> fftForward, fftInverse;
std::unique_ptr<numeric::Operation> polynomial;
std::unique_ptr<numeric::Operation> fit;
std::atomic<bool> ready(false);
struct Stream {
    std::unique_ptr<numeric::Operation> operation;
    double sequence = 0;
    double reset = 0;
    bool hasSequence = false;
};
std::mutex streamsMutex;
std::map<aSubRecord*, std::unique_ptr<Stream>> streams;
void releaseStream(aSubRecord* rec) {
    std::lock_guard<std::mutex> guard(streamsMutex);
    streams.erase(rec);
}
bool token(double value) {
    return std::isfinite(value) && value >= 0 && value <= 9007199254740992.0 &&
           std::floor(value) == value;
}
bool scalarLink(const DBLINK& link) {
    if (link.type == CONSTANT) return true;
    long elements = 0;
    return dbGetNelements(&link, &elements) == 0 && elements == 1;
}
void setup() {
    try {
        std::call_once(setupOnce, [] {
            numeric::initializeGsl();
            numeric::Registry registry;
            numeric::registerBuiltins(registry);
            auto operation = registry.create("mean");
            auto varianceOperation = registry.create("variance");
            auto stddevOperation = registry.create("stddev");
            auto popVarOperation = registry.create("population_variance");
            auto popSdOperation = registry.create("population_stddev");
            auto minmaxOperation = registry.create("minmax");
            auto covarianceOperation = registry.create("covariance");
            auto correlationOperation = registry.create("correlation");
            auto fftForwardOperation = registry.create("fft_complex_radix2_forward");
            auto fftInverseOperation = registry.create("fft_complex_radix2_inverse");
            auto polynomialOperation = registry.create("polynomial_eval");
            auto fitOperation = registry.create("linear_fit");
            if (!fitOperation || !operation || !varianceOperation || !stddevOperation || !popVarOperation || !popSdOperation || !minmaxOperation || !covarianceOperation || !correlationOperation || !fftForwardOperation || !fftInverseOperation || !polynomialOperation) throw std::bad_alloc();
            mean = std::move(operation);
            variance = std::move(varianceOperation);
            stddev = std::move(stddevOperation);
            populationVariance = std::move(popVarOperation);
            populationStddev = std::move(popSdOperation);
            minmax = std::move(minmaxOperation);
            covariance = std::move(covarianceOperation);
            correlation = std::move(correlationOperation);
            fftForward = std::move(fftForwardOperation);
            fftInverse = std::move(fftInverseOperation);
            polynomial = std::move(polynomialOperation);
            fit = std::move(fitOperation);
            ready.store(true, std::memory_order_release);
        });
    } catch (...) {
        std::fprintf(stderr, "numeric: initialization failed\n");
    }
}
const iocshFuncDef setupDef = {"numericInitialize", 0, nullptr};
void setupCall(const iocshArgBuf*) { setup(); }
void registerSetup() { iocshRegister(&setupDef, setupCall); }
}
bool numericIocReady() { return ready.load(std::memory_order_acquire); }
extern "C" long numericLinearFit(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = 0;
    try {
        if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE || rec->ftb != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || !rec->a || !rec->b || !rec->vala || rec->nova < 6 ||
            rec->inpa.type == CONSTANT || rec->inpb.type == CONSTANT ||
            dbGetLinkDBFtype(&rec->inpa) != DBF_DOUBLE || dbGetLinkDBFtype(&rec->inpb) != DBF_DOUBLE ||
            rec->nea >= rec->noa || rec->neb >= rec->nob) throw 1;
        if (rec->outa.type != CONSTANT) {
            long capacity = 0;
            if (dbGetLinkDBFtype(&rec->outa) != DBF_DOUBLE || dbGetNelements(&rec->outa, &capacity) || capacity < 6) throw 1;
        }
        numeric::Input inputs[] = {
            {static_cast<const double*>(rec->a), rec->nea, rec->noa, 1, numeric::Type::Float64, numeric::Shape::Array},
            {static_cast<const double*>(rec->b), rec->neb, rec->nob, 1, numeric::Type::Float64, numeric::Shape::Array}};
        numeric::Output output{static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Array};
        if (fit->execute(inputs, 2, &output, 1).status != numeric::Status::Ok) throw 1;
        rec->neva = output.count; rec->udf = FALSE; return 0;
    } catch (...) { recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1; }
}
epicsRegisterFunction(numericLinearFit);
extern "C" long numericPolynomial(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = 0;
    try {
        if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE || rec->ftb != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || !rec->a || !rec->b || !rec->vala || rec->nova < 1 ||
            rec->inpa.type == CONSTANT || dbGetLinkDBFtype(&rec->inpa) != DBF_DOUBLE ||
            (rec->inpb.type != CONSTANT && dbGetLinkDBFtype(&rec->inpb) != DBF_DOUBLE) ||
            rec->nea >= rec->noa || rec->nob != 1 || rec->neb != 1 || !scalarLink(rec->inpb)) throw 1;
        if (rec->outa.type != CONSTANT) {
            long capacity = 0;
            if (dbGetLinkDBFtype(&rec->outa) != DBF_DOUBLE || dbGetNelements(&rec->outa, &capacity) || capacity != 1) throw 1;
        }
        numeric::Input inputs[] = {
            {static_cast<const double*>(rec->a), rec->nea, rec->noa, 1, numeric::Type::Float64, numeric::Shape::Array},
            {static_cast<const double*>(rec->b), 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar}};
        numeric::Output output{static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Scalar};
        if (polynomial->execute(inputs, 2, &output, 1).status != numeric::Status::Ok) throw 1;
        rec->neva = output.count; rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
    }
}
epicsRegisterFunction(numericPolynomial);
static long numericFft(aSubRecord* rec, bool inverse) {
    if (!rec) return -1;
    rec->neva = 0;
    try {
        if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || !rec->a || !rec->vala ||
            rec->inpa.type == CONSTANT || rec->nea >= rec->noa || rec->nea > rec->nova) throw 1;
        if (rec->outa.type != CONSTANT) {
            long capacity = 0;
            if (dbGetLinkDBFtype(&rec->outa) != DBF_DOUBLE ||
                dbGetNelements(&rec->outa, &capacity) || capacity < 0 ||
                static_cast<std::size_t>(capacity) < rec->nea) throw 1;
        }
        double* scratch = static_cast<double*>(rec->a);
        numeric::Input input{scratch, rec->nea, rec->noa, 1, numeric::Type::Float64, numeric::Shape::Array};
        numeric::Output output{scratch, rec->noa, 0, numeric::Type::Float64, numeric::Shape::Array};
        numeric::Operation* operation = inverse ? fftInverse.get() : fftForward.get();
        if (operation->execute(&input, 1, &output, 1).status != numeric::Status::Ok) throw 1;
        double* destination = static_cast<double*>(rec->vala);
        for (std::size_t i = 0; i < output.count; ++i) destination[i] = scratch[i];
        rec->neva = output.count;
        rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
extern "C" long numericFftForward(aSubRecord* rec) { return numericFft(rec, false); }
extern "C" long numericFftInverse(aSubRecord* rec) { return numericFft(rec, true); }
epicsRegisterFunction(numericFftForward);
epicsRegisterFunction(numericFftInverse);
long numericBatch(aSubRecord* rec, int kind) {
    if (!rec) return -1;
    rec->neva = 0;
    if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE ||
        rec->ftva != DBF_DOUBLE || rec->nova < 1 || !rec->a || !rec->vala ||
        rec->nea >= rec->noa) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
    }
    try {
        numeric::Operation* operation = kind == 0 ? mean.get() : kind == 1 ? variance.get() :
                                        kind == 2 ? stddev.get() : kind == 3 ? populationVariance.get() : populationStddev.get();
        numeric::Input input{static_cast<const double*>(rec->a), rec->nea, rec->noa, 1,
                             numeric::Type::Float64, numeric::Shape::Array};
        numeric::Output output{static_cast<double*>(rec->vala), rec->nova, 0,
                               numeric::Type::Float64, numeric::Shape::Scalar};
        const numeric::Result result = operation->execute(&input, 1, &output, 1);
        if (result.status != numeric::Status::Ok) {
            recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
        }
        rec->neva = output.count;
        rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
    }
}
extern "C" long numericMean(aSubRecord* rec) { return numericBatch(rec, 0); }
extern "C" long numericVariance(aSubRecord* rec) { return numericBatch(rec, 1); }
extern "C" long numericStddev(aSubRecord* rec) { return numericBatch(rec, 2); }
extern "C" long numericPopulationVariance(aSubRecord* rec) { return numericBatch(rec, 3); }
extern "C" long numericPopulationStddev(aSubRecord* rec) { return numericBatch(rec, 4); }
epicsRegisterFunction(numericMean);
epicsRegisterFunction(numericVariance);
epicsRegisterFunction(numericStddev);
epicsRegisterFunction(numericPopulationVariance);
epicsRegisterFunction(numericPopulationStddev);
extern "C" long numericMinmax(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = rec->nevb = 0;
    try {
        if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || rec->ftvb != DBF_DOUBLE || !rec->a || !rec->vala || !rec->valb ||
            rec->nova < 1 || rec->novb < 1 || rec->nea >= rec->noa) throw 1;
        numeric::Input input{static_cast<const double*>(rec->a), rec->nea, rec->noa, 1,
                             numeric::Type::Float64, numeric::Shape::Array};
        numeric::Output outputs[] = {
            {static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->valb), rec->novb, 0, numeric::Type::Float64, numeric::Shape::Scalar}};
        if (minmax->execute(&input, 1, outputs, 2).status != numeric::Status::Ok) throw 1;
        rec->neva = rec->nevb = 1;
        rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
epicsRegisterFunction(numericMinmax);
static long numericTwoArrayStatistic(aSubRecord* rec, bool useCorrelation) {
    if (!rec) return -1;
    rec->neva = 0;
    try {
        if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE || rec->ftb != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || !rec->a || !rec->b || !rec->vala || rec->nova < 1 ||
            rec->nea >= rec->noa || rec->neb >= rec->nob) throw 1;
        numeric::Input inputs[] = {
            {static_cast<const double*>(rec->a), rec->nea, rec->noa, 1, numeric::Type::Float64, numeric::Shape::Array},
            {static_cast<const double*>(rec->b), rec->neb, rec->nob, 1, numeric::Type::Float64, numeric::Shape::Array}};
        numeric::Output output{static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Scalar};
        numeric::Operation* operation = useCorrelation ? correlation.get() : covariance.get();
        if (operation->execute(inputs, 2, &output, 1).status != numeric::Status::Ok) throw 1;
        rec->neva = 1; rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
extern "C" long numericCovariance(aSubRecord* rec) { return numericTwoArrayStatistic(rec, false); }
extern "C" long numericCorrelation(aSubRecord* rec) { return numericTwoArrayStatistic(rec, true); }
epicsRegisterFunction(numericCovariance);
epicsRegisterFunction(numericCorrelation);
static long numericPairedStatistic(aSubRecord* rec, bool useCorrelation) {
    if (!rec) return -1;
    rec->neva = 0;
    try {
        if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE || rec->ftva != DBF_DOUBLE ||
            !rec->a || !rec->vala || rec->nova < 1 || rec->nea >= rec->noa || rec->nea < 4 || rec->nea % 2) throw 1;
        const double* data = static_cast<const double*>(rec->a);
        const std::size_t count = rec->nea / 2;
        numeric::Input inputs[] = {
            {data, count, rec->noa, 2, numeric::Type::Float64, numeric::Shape::Array},
            {data + 1, count, rec->noa - 1, 2, numeric::Type::Float64, numeric::Shape::Array}};
        numeric::Output output{static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Scalar};
        numeric::Operation* operation = useCorrelation ? correlation.get() : covariance.get();
        if (operation->execute(inputs, 2, &output, 1).status != numeric::Status::Ok) throw 1;
        rec->neva = 1; rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
extern "C" long numericCovariancePairs(aSubRecord* rec) { return numericPairedStatistic(rec, false); }
extern "C" long numericCorrelationPairs(aSubRecord* rec) { return numericPairedStatistic(rec, true); }
epicsRegisterFunction(numericCovariancePairs);
epicsRegisterFunction(numericCorrelationPairs);
long numericRunning(aSubRecord* rec, bool statistics) {
    if (!rec) return -1;
    rec->neva = rec->nevb = 0;
    if (statistics) rec->nevc = rec->nevd = rec->neve = rec->nevf = rec->nevg = 0;
    try {
        if (!ready.load(std::memory_order_acquire) ||
            rec->fta != DBF_DOUBLE || rec->ftb != DBF_DOUBLE || rec->ftc != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || rec->ftvb != DBF_DOUBLE ||
            rec->noa != 1 || rec->nob != 1 || rec->noc != 1 ||
            rec->nea != 1 || rec->neb != 1 || rec->nec != 1 ||
            rec->nova < 1 || rec->novb < 1 || !rec->a || !rec->b || !rec->c ||
            !rec->vala || !rec->valb || !scalarLink(rec->inpa) ||
            !scalarLink(rec->inpb) || !scalarLink(rec->inpc)) throw 1;
        if (statistics && (rec->ftvc != DBF_DOUBLE || rec->ftvd != DBF_DOUBLE ||
            rec->ftve != DBF_DOUBLE || rec->ftvf != DBF_DOUBLE || rec->ftvg != DBF_DOUBLE ||
            rec->novc < 1 || rec->novd < 1 || rec->nove < 1 || rec->novf < 1 || rec->novg < 1 ||
            !rec->valc || !rec->vald || !rec->vale || !rec->valf || !rec->valg)) throw 1;
        const double sample = *static_cast<double*>(rec->a);
        const double sequence = *static_cast<double*>(rec->b);
        const double reset = *static_cast<double*>(rec->c);
        if (!token(sequence) || !token(reset)) throw 1;
        Stream* stream;
        {
            std::lock_guard<std::mutex> guard(streamsMutex);
            auto found = streams.find(rec);
            if (found == streams.end()) {
                numeric::Registry registry;
                numeric::registerBuiltins(registry);
                std::unique_ptr<Stream> created(new Stream);
                created->operation = registry.create(statistics ? "running_stats" : "running_mean");
                if (!created->operation) throw 1;
                created->reset = reset;
                found = streams.emplace(rec, std::move(created)).first;
                rec->cadr = &releaseStream;
            }
            stream = found->second.get();
        }
        if (reset < stream->reset) throw 1;
        if (reset > stream->reset) {
            stream->operation->reset();
            stream->reset = reset;
            *static_cast<double*>(rec->vala) = std::numeric_limits<double>::quiet_NaN();
            *static_cast<double*>(rec->valb) = statistics ? std::numeric_limits<double>::quiet_NaN() : 0;
            rec->neva = rec->nevb = 1;
            if (statistics) {
                *static_cast<double*>(rec->valc) = std::numeric_limits<double>::quiet_NaN();
                *static_cast<double*>(rec->vald) = std::numeric_limits<double>::quiet_NaN();
                *static_cast<double*>(rec->vale) = std::numeric_limits<double>::quiet_NaN();
                *static_cast<double*>(rec->valf) = std::numeric_limits<double>::quiet_NaN();
                *static_cast<double*>(rec->valg) = 0;
                rec->nevc = rec->nevd = rec->neve = rec->nevf = rec->nevg = 1;
            }
            rec->udf = FALSE;
            return 0;
        }
        if (stream->hasSequence && sequence < stream->sequence) throw 1;
        if (stream->hasSequence && sequence == stream->sequence) return 1;
        if (statistics && stream->hasSequence && sequence != stream->sequence + 1.0) throw 1;
        numeric::Input input{&sample, 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar};
        numeric::Output outputs[] = {
            {static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->valb), rec->novb, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->valc), rec->novc, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->vald), rec->novd, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->vale), rec->nove, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->valf), rec->novf, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->valg), rec->novg, 0, numeric::Type::Float64, numeric::Shape::Scalar}
        };
        if (stream->operation->execute(&input, 1, outputs, statistics ? 7 : 2).status != numeric::Status::Ok) throw 1;
        stream->sequence = sequence;
        stream->hasSequence = true;
        rec->neva = rec->nevb = 1;
        if (statistics) rec->nevc = rec->nevd = rec->neve = rec->nevf = rec->nevg = 1;
        rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
extern "C" long numericRunningMean(aSubRecord* rec) { return numericRunning(rec, false); }
extern "C" long numericRunningStats(aSubRecord* rec) { return numericRunning(rec, true); }
epicsRegisterFunction(numericRunningMean);
epicsRegisterFunction(numericRunningStats);
epicsExportRegistrar(registerSetup);

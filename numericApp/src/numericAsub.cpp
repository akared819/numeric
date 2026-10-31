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
            if (!operation) throw std::bad_alloc();
            mean = std::move(operation);
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
extern "C" long numericMean(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = 0;
    if (!ready.load(std::memory_order_acquire) || rec->fta != DBF_DOUBLE ||
        rec->ftva != DBF_DOUBLE || rec->nova < 1 || !rec->a || !rec->vala ||
        rec->nea >= rec->noa) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
    }
    try {
        numeric::Input input{static_cast<const double*>(rec->a), rec->nea, rec->noa, 1,
                             numeric::Type::Float64, numeric::Shape::Array};
        numeric::Output output{static_cast<double*>(rec->vala), rec->nova, 0,
                               numeric::Type::Float64, numeric::Shape::Scalar};
        const numeric::Result result = mean->execute(&input, 1, &output, 1);
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
epicsRegisterFunction(numericMean);
extern "C" long numericRunningMean(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = rec->nevb = 0;
    try {
        if (!ready.load(std::memory_order_acquire) ||
            rec->fta != DBF_DOUBLE || rec->ftb != DBF_DOUBLE || rec->ftc != DBF_DOUBLE ||
            rec->ftva != DBF_DOUBLE || rec->ftvb != DBF_DOUBLE ||
            rec->noa != 1 || rec->nob != 1 || rec->noc != 1 ||
            rec->nea != 1 || rec->neb != 1 || rec->nec != 1 ||
            rec->nova < 1 || rec->novb < 1 || !rec->a || !rec->b || !rec->c ||
            !rec->vala || !rec->valb || !scalarLink(rec->inpa) ||
            !scalarLink(rec->inpb) || !scalarLink(rec->inpc)) throw 1;
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
                created->operation = registry.create("running_mean");
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
            *static_cast<double*>(rec->valb) = 0;
            rec->neva = rec->nevb = 1;
            rec->udf = FALSE;
            return 0;
        }
        if (stream->hasSequence && sequence < stream->sequence) throw 1;
        if (stream->hasSequence && sequence == stream->sequence) return 1;
        numeric::Input input{&sample, 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar};
        numeric::Output outputs[] = {
            {static_cast<double*>(rec->vala), rec->nova, 0, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<double*>(rec->valb), rec->novb, 0, numeric::Type::Float64, numeric::Shape::Scalar}
        };
        if (stream->operation->execute(&input, 1, outputs, 2).status != numeric::Status::Ok) throw 1;
        stream->sequence = sequence;
        stream->hasSequence = true;
        rec->neva = rec->nevb = 1;
        rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
epicsRegisterFunction(numericRunningMean);
epicsExportRegistrar(registerSetup);

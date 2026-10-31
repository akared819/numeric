// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <aSubRecord.h>
#include <alarm.h>
#include <dbFldTypes.h>
#include <epicsExport.h>
#include <iocsh.h>
#include <recGbl.h>
#include <registryFunction.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <cstdio>

namespace {
std::once_flag setupOnce;
std::unique_ptr<numeric::Operation> mean;
std::atomic<bool> ready(false);
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
epicsExportRegistrar(registerSetup);

// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <aSubRecord.h>
#include <alarm.h>
#include <dbFldTypes.h>
#include <dbLink.h>
#include <epicsExport.h>
#include <recGbl.h>
#include <registryFunction.h>
#include <map>
#include <memory>
#include <mutex>

bool numericIocReady();
namespace {
std::mutex instancesMutex;
std::map<aSubRecord*, std::unique_ptr<numeric::Operation>> instances;
void release(aSubRecord* rec) {
    std::lock_guard<std::mutex> lock(instancesMutex);
    instances.erase(rec);
}
bool scalarLink(const DBLINK& link) {
    if (link.type == CONSTANT) return true;
    long count = 0;
    return !dbGetNelements(&link, &count) && count == 1;
}
long calculate(aSubRecord* rec, bool gaussian) {
    if (!rec) return -1;
    rec->neva = 0;
    try {
        if (!numericIocReady() || rec->fta != DBF_DOUBLE || rec->ftb != DBF_DOUBLE ||
            (gaussian && rec->ftc != DBF_DOUBLE) || rec->ftva != DBF_DOUBLE ||
            rec->inpa.type == CONSTANT || !rec->a || !rec->b || !rec->vala ||
            (gaussian && !rec->c) || rec->nea >= rec->noa || rec->nea > rec->nova ||
            rec->nob != 1 || rec->neb != 1 || !scalarLink(rec->inpb) ||
            (gaussian && (rec->noc != 1 || rec->nec != 1 || !scalarLink(rec->inpc)))) throw 1;
        if (rec->outa.type != CONSTANT) {
            long capacity = 0;
            if (dbGetLinkDBFtype(&rec->outa) != DBF_DOUBLE || dbGetNelements(&rec->outa, &capacity) ||
                capacity < 0 || static_cast<std::size_t>(capacity) < rec->nea) throw 1;
        }
        numeric::Operation* operation;
        {
            std::lock_guard<std::mutex> lock(instancesMutex);
            auto it = instances.find(rec);
            if (it == instances.end()) {
                numeric::Registry registry; numeric::registerBuiltins(registry);
                auto op = registry.create(gaussian ? "gaussian_smooth" : "moving_mean");
                if (!op) throw 1;
                it = instances.emplace(rec, std::move(op)).first;
                rec->cadr = &release;
            }
            operation = it->second.get();
        }
        double* scratch = static_cast<double*>(rec->a);
        numeric::Input input[] = {
            {scratch, rec->nea, rec->noa, 1, numeric::Type::Float64, numeric::Shape::Array},
            {static_cast<const double*>(rec->b), 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar},
            {static_cast<const double*>(rec->c), 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar}};
        numeric::Output output{scratch, rec->noa, 0, numeric::Type::Float64, numeric::Shape::Array};
        if (operation->execute(input, gaussian ? 3 : 2, &output, 1).status != numeric::Status::Ok) throw 1;
        double* dest = static_cast<double*>(rec->vala);
        for (std::size_t i = 0; i < output.count; ++i) dest[i] = scratch[i];
        rec->neva = output.count; rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
    }
}
}
extern "C" long numericMovingMean(aSubRecord* rec) { return calculate(rec, false); }
extern "C" long numericGaussianSmooth(aSubRecord* rec) { return calculate(rec, true); }
epicsRegisterFunction(numericMovingMean);
epicsRegisterFunction(numericGaussianSmooth);

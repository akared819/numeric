// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include <aSubRecord.h>
#include <alarm.h>
#include <dbFldTypes.h>
#include <dbLink.h>
#include <recGbl.h>
#include <registryFunction.h>
#include <epicsExport.h> // Must follow all imported EPICS declarations.
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
}
extern "C" long numericRealFftForward(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = rec->nevb = 0;
    try {
        const std::size_t n = rec->nea;
        if (!numericIocReady() || rec->fta != DBF_DOUBLE || rec->ftva != DBF_DOUBLE ||
            rec->ftvb != DBF_DOUBLE || !rec->a || !rec->vala || !rec->valb ||
            rec->inpa.type == CONSTANT || rec->outb.type != CONSTANT ||
            rec->nea >= rec->noa || n > static_cast<std::size_t>(rec->nova) / 2 ||
            n > static_cast<std::size_t>(rec->novb) / 2) throw 1;
        if (rec->outa.type != CONSTANT) {
            long capacity = 0;
            if (dbGetLinkDBFtype(&rec->outa) != DBF_DOUBLE || dbGetNelements(&rec->outa, &capacity) ||
                capacity < 0 || n > static_cast<std::size_t>(capacity) / 2) throw 1;
        }
        numeric::Operation* operation;
        {
            std::lock_guard<std::mutex> lock(instancesMutex);
            auto it = instances.find(rec);
            if (it == instances.end()) {
                numeric::Registry registry; numeric::registerBuiltins(registry);
                auto op = registry.create("fft_real_radix2_forward");
                if (!op) throw 1;
                it = instances.emplace(rec, std::move(op)).first;
                rec->cadr = &release;
            }
            operation = it->second.get();
        }
        numeric::Input input{static_cast<const double*>(rec->a), n, rec->noa, 1,
                             numeric::Type::Float64, numeric::Shape::Array};
        numeric::Output scratch{static_cast<double*>(rec->valb), rec->novb, 0,
                                numeric::Type::Float64, numeric::Shape::Array};
        if (operation->execute(&input, 1, &scratch, 1).status != numeric::Status::Ok) throw 1;
        double* dest = static_cast<double*>(rec->vala);
        for (std::size_t i = 0; i < scratch.count; ++i) dest[i] = scratch.data[i];
        rec->neva = scratch.count; rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM); return -1;
    }
}
epicsRegisterFunction(numericRealFftForward);

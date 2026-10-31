// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAck.h"
#include <aSubRecord.h>
#include <alarm.h>
#include <dbFldTypes.h>
#include <epicsExport.h>
#include <recGbl.h>
#include <registryFunction.h>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>

bool numericIocReady();
namespace {
std::mutex statesMutex;
std::map<aSubRecord*, std::unique_ptr<numeric::AcknowledgedStream>> states;
void releaseAck(aSubRecord* record) {
    std::lock_guard<std::mutex> guard(statesMutex);
    states.erase(record);
}
bool token(double x, bool nonzero) {
    return std::isfinite(x) && x >= (nonzero ? 1.0 : 0.0) &&
           x <= 9007199254740992.0 && std::floor(x) == x;
}
}
extern "C" long numericAcknowledgedStats(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva = rec->nevb = rec->nevc = rec->nevd = rec->neve = 0;
    rec->nevf = rec->nevg = rec->nevh = rec->nevi = rec->nevj = rec->nevk = 0;
    try {
        if (!numericIocReady() || rec->fta != DBF_DOUBLE || !rec->a || rec->noa < 5 || rec->nea != 4 ||
            rec->ftva != DBF_DOUBLE || rec->ftvb != DBF_DOUBLE || rec->ftvc != DBF_DOUBLE ||
            rec->ftvd != DBF_DOUBLE || rec->ftve != DBF_DOUBLE || rec->ftvf != DBF_DOUBLE ||
            rec->ftvg != DBF_DOUBLE || rec->ftvh != DBF_DOUBLE || rec->ftvi != DBF_DOUBLE ||
            rec->ftvj != DBF_DOUBLE || rec->ftvk != DBF_DOUBLE ||
            !rec->vala || !rec->valb || !rec->valc || !rec->vald || !rec->vale || !rec->valf ||
            !rec->valg || !rec->valh || !rec->vali || !rec->valj || !rec->valk || rec->nova < 1 || rec->novb < 1 ||
            rec->novc < 1 || rec->novd < 1 || rec->nove < 1 || rec->novf < 1 || rec->novg < 1 ||
            rec->novh < 1 || rec->novi < 1 || rec->novj < 1 || rec->novk < 1) throw 1;
        const double* payload = static_cast<const double*>(rec->a);
        if (!token(payload[0], true) || !token(payload[1], false) ||
            (payload[2] != 0 && payload[2] != 1) || !std::isfinite(payload[3])) throw 1;
        const numeric::Event event{static_cast<std::uint64_t>(payload[0]), static_cast<std::uint64_t>(payload[1]),
            payload[2] == 0 ? numeric::EventKind::Sample : numeric::EventKind::Reset, payload[3]};
        numeric::AcknowledgedStream* stream;
        {
            std::lock_guard<std::mutex> guard(statesMutex);
            auto found = states.find(rec);
            if (found == states.end()) {
                auto created = numeric::AcknowledgedStream::create();
                if (!created) throw 1;
                found = states.emplace(rec, std::move(created)).first;
                rec->cadr = &releaseAck;
            }
            stream = found->second.get();
        }
        numeric::Acknowledgment acknowledgment;
        if (stream->submit(event, acknowledgment).status != numeric::Status::Ok) throw 1;
        void* values[] = {rec->vala, rec->valb, rec->valc, rec->vald, rec->vale,
                          rec->valf, rec->valg, rec->valh, rec->vali};
        for (std::size_t i = 0; i < 7; ++i) *static_cast<double*>(values[i]) = acknowledgment.statistics[i];
        *static_cast<double*>(values[7]) = static_cast<double>(acknowledgment.event.session);
        *static_cast<double*>(values[8]) = static_cast<double>(acknowledgment.event.sequence);
        *static_cast<double*>(rec->valj) = acknowledgment.event.kind == numeric::EventKind::Sample ? 0.0 : 1.0;
        *static_cast<double*>(rec->valk) = acknowledgment.event.value;
        rec->neva = rec->nevb = rec->nevc = rec->nevd = rec->neve = 1;
        rec->nevf = rec->nevg = rec->nevh = rec->nevi = rec->nevj = rec->nevk = 1;
        rec->udf = FALSE;
        return 0;
    } catch (...) {
        recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
        return -1;
    }
}
epicsRegisterFunction(numericAcknowledgedStats);

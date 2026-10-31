// SPDX-License-Identifier: GPL-3.0-or-later
#define USE_TYPED_RSET
#include "numeric.h"
#include <alarm.h>
#include <dbAccess.h>
#include <dbEvent.h>
#include <dbFldTypes.h>
#include <dbLink.h>
#include <epicsExit.h>
#include <recGbl.h>
#include <recSup.h>
#define GEN_SIZE_OFFSET
#include "numericScalarRecord.h"
#undef GEN_SIZE_OFFSET
#include <epicsExport.h>
#include <memory>
#include <vector>

namespace {
struct State {
    std::vector<double> buffer;
    std::unique_ptr<numeric::Operation> op;
};
void destroyState(void* arg) { delete static_cast<State*>(arg); }
long initRecord(dbCommon* common, int pass) {
    if (pass == 0) return 0;
    numericScalarRecord* rec = reinterpret_cast<numericScalarRecord*>(common);
    try {
        if (rec->nmax < 1 || rec->nmax > 1048576 || rec->inp.type == CONSTANT) return S_db_badField;
        numeric::Registry registry; numeric::registerBuiltins(registry);
        const numeric::Metadata* m = registry.find(rec->op);
        if (!m || m->stateful || m->inputs.size() != 1 || m->outputs.size() != 1 ||
            m->inputs[0].type != numeric::Type::Float64 || m->inputs[0].shape != numeric::Shape::Array ||
            m->outputs[0].type != numeric::Type::Float64 || m->outputs[0].shape != numeric::Shape::Scalar) return S_db_badField;
        std::unique_ptr<State> state(new State);
        state->buffer.resize(static_cast<std::size_t>(rec->nmax) + 1);
        state->op = registry.create(rec->op);
        if (!state->op || epicsAtExit(destroyState, state.get())) return S_rec_outMem;
        rec->rpvt = state.release();
        return 0;
    } catch (...) { return S_rec_outMem; }
}
long process(dbCommon* common) {
    numericScalarRecord* rec = reinterpret_cast<numericScalarRecord*>(common);
    rec->pact = TRUE;
    State* state = static_cast<State*>(rec->rpvt);
    bool ok = false;
    epicsUInt32 oldCount = rec->nord;
    rec->nord = 0;
    try {
        long count = static_cast<long>(rec->nmax) + 1;
        if (state && dbGetLinkDBFtype(&rec->inp) == DBF_DOUBLE &&
            dbGetLink(&rec->inp, DBR_DOUBLE, state->buffer.data(), nullptr, &count) == 0 &&
            count > 0 && static_cast<unsigned long>(count) <= rec->nmax) {
            double value = 0;
            numeric::Input input{state->buffer.data(), static_cast<std::size_t>(count), state->buffer.size(), 1,
                                 numeric::Type::Float64, numeric::Shape::Array};
            numeric::Output output{&value, 1, 0, numeric::Type::Float64, numeric::Shape::Scalar};
            if (state->op->execute(&input, 1, &output, 1).status == numeric::Status::Ok) {
                rec->val = value; rec->nord = static_cast<epicsUInt32>(count); rec->udf = FALSE; ok = true;
            }
        }
    } catch (...) {}
    if (!ok) recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
    recGblGetTimeStamp(rec);
    unsigned int mask = recGblResetAlarms(rec);
    if (ok) mask |= DBE_VALUE | DBE_LOG;
    if (mask) db_post_events(rec, &rec->val, mask);
    if (oldCount != rec->nord) db_post_events(rec, &rec->nord, DBE_VALUE | DBE_LOG);
    recGblFwdLink(rec);
    rec->pact = FALSE;
    return 0;
}
long precision(const DBADDR* addr, long* value) {
    const numericScalarRecord* rec = reinterpret_cast<const numericScalarRecord*>(addr->precord);
    *value = rec->prec;
    return 0;
}
}
extern "C" {
rset numericScalarRSET = {RSETNUMBER, nullptr, nullptr, initRecord, process, nullptr, nullptr, nullptr,
    nullptr, nullptr, nullptr, precision, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
epicsExportAddress(rset, numericScalarRSET);
}

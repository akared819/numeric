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
#include <special.h>
#define GEN_SIZE_OFFSET
#include "numericArrayRecord.h"
#undef GEN_SIZE_OFFSET
#include <epicsExport.h>
#include <cstring>
#include <memory>
#include <vector>

namespace {
struct State {
    std::vector<double> input, staging, output;
    std::unique_ptr<numeric::Operation> op;
    std::size_t parameters;
};
void destroyState(void* arg) { delete static_cast<State*>(arg); }
int parameterCount(const char* name) {
    if (!std::strcmp(name, "fft_complex_radix2_forward") || !std::strcmp(name, "fft_complex_radix2_inverse") ||
        !std::strcmp(name, "fft_real_radix2_forward")) return 0;
    if (!std::strcmp(name, "moving_mean") || !std::strcmp(name, "moving_median")) return 1;
    if (!std::strcmp(name, "gaussian_smooth")) return 2;
    if (!std::strcmp(name, "histogram_uniform")) return 3;
    return -1;
}
long initRecord(dbCommon* common, int pass) {
    if (pass != 0) return 0;
    numericArrayRecord* rec = reinterpret_cast<numericArrayRecord*>(common);
    try {
        const int params = parameterCount(rec->op);
        if (rec->nmax < 1 || rec->nmax > 1048576 || rec->omax < 1 || rec->omax > 1048576 ||
            rec->inp.type == CONSTANT || params < 0) return S_db_badField;
        numeric::Registry registry; numeric::registerBuiltins(registry);
        const numeric::Metadata* m = registry.find(rec->op);
        if (!m || m->stateful || m->inputs.size() != static_cast<std::size_t>(params + 1) ||
            m->outputs.size() != 1 || m->inputs[0].type != numeric::Type::Float64 ||
            m->inputs[0].shape != numeric::Shape::Array || m->outputs[0].type != numeric::Type::Float64 ||
            m->outputs[0].shape != numeric::Shape::Array) return S_db_badField;
        for (int i = 1; i <= params; ++i)
            if (m->inputs[i].type != numeric::Type::Float64 || m->inputs[i].shape != numeric::Shape::Scalar) return S_db_badField;
        std::unique_ptr<State> state(new State);
        state->input.resize(static_cast<std::size_t>(rec->nmax) + 1);
        state->staging.resize(rec->omax); state->output.resize(rec->omax);
        state->op = registry.create(rec->op); state->parameters = params;
        if (!state->op || epicsAtExit(destroyState, state.get())) return S_rec_outMem;
        rec->rpvt = state.release();
        return 0;
    } catch (...) { return S_rec_outMem; }
}
long process(dbCommon* common) {
    numericArrayRecord* rec = reinterpret_cast<numericArrayRecord*>(common);
    rec->pact = TRUE;
    State* state = static_cast<State*>(rec->rpvt);
    bool ok = false;
    const epicsUInt32 oldInput = rec->nin, oldOutput = rec->nord;
    rec->nin = 0;
    try {
        long count = static_cast<long>(rec->nmax) + 1;
        if (state && dbGetLinkDBFtype(&rec->inp) == DBF_DOUBLE &&
            dbGetLink(&rec->inp, DBR_DOUBLE, state->input.data(), nullptr, &count) == 0 &&
            count > 0 && static_cast<unsigned long>(count) <= rec->nmax) {
            numeric::Input inputs[] = {
                {state->input.data(), static_cast<std::size_t>(count), state->input.size(), 1, numeric::Type::Float64, numeric::Shape::Array},
                {state->parameters == 3 ? &rec->hmin : &rec->win, 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar},
                {state->parameters == 3 ? &rec->hmax : &rec->alph, 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar},
                {&rec->nbin, 1, 1, 1, numeric::Type::Float64, numeric::Shape::Scalar}};
            numeric::Output output{state->staging.data(), state->staging.size(), 0, numeric::Type::Float64, numeric::Shape::Array};
            if (state->op->execute(inputs, 1 + state->parameters, &output, 1).status == numeric::Status::Ok) {
                for (std::size_t i = 0; i < output.count; ++i) state->output[i] = state->staging[i];
                rec->nin = static_cast<epicsUInt32>(count); rec->nord = static_cast<epicsUInt32>(output.count);
                rec->udf = FALSE; ok = true;
            }
        }
    } catch (...) {}
    if (!ok) recGblSetSevr(rec, CALC_ALARM, INVALID_ALARM);
    recGblGetTimeStamp(rec);
    unsigned int mask = recGblResetAlarms(rec);
    if (ok) mask |= DBE_VALUE | DBE_LOG;
    if (mask) db_post_events(rec, &rec->val, mask);
    if (oldInput != rec->nin) db_post_events(rec, &rec->nin, DBE_VALUE | DBE_LOG);
    if (oldOutput != rec->nord) db_post_events(rec, &rec->nord, DBE_VALUE | DBE_LOG);
    recGblFwdLink(rec); rec->pact = FALSE;
    return 0;
}
long cvtAddress(DBADDR* addr) {
    numericArrayRecord* rec = reinterpret_cast<numericArrayRecord*>(addr->precord);
    State* state = static_cast<State*>(rec->rpvt);
    if (!state) return S_db_badField;
    addr->no_elements = rec->omax;
    addr->field_type = DBF_DOUBLE; addr->field_size = sizeof(double);
    addr->dbr_field_type = DBR_DOUBLE; addr->special = SPC_NOMOD;
    return 0;
}
long arrayInfo(DBADDR* addr, long* count, long* offset) {
    numericArrayRecord* rec = reinterpret_cast<numericArrayRecord*>(addr->precord);
    State* state = static_cast<State*>(rec->rpvt);
    if (!state) return S_db_badField;
    addr->pfield = state->output.data(); *count = rec->nord; *offset = 0;
    return 0;
}
long precision(const DBADDR* addr, long* value) {
    *value = reinterpret_cast<const numericArrayRecord*>(addr->precord)->prec;
    return 0;
}
}
extern "C" {
rset numericArrayRSET = {RSETNUMBER, nullptr, nullptr, initRecord, process, nullptr, nullptr, cvtAddress,
    arrayInfo, nullptr, nullptr, precision, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
epicsExportAddress(rset, numericArrayRSET);
}

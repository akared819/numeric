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
    std::vector<std::vector<double>> buffers;
    std::vector<numeric::Input> inputs;
    std::vector<DBLINK*> links;
    std::vector<numeric::Shape> shapes;
    std::unique_ptr<numeric::Operation> op;
};
void destroyState(void* arg) { delete static_cast<State*>(arg); }
long initRecord(dbCommon* common, int pass) {
    if (pass == 0) return 0;
    numericScalarRecord* rec = reinterpret_cast<numericScalarRecord*>(common);
    DBLINK* namedLinks[] = {&rec->inpa, &rec->inpb, &rec->inpc, &rec->inpd, &rec->inpe, &rec->inpf, &rec->inpg, &rec->inph,
                            &rec->inpi, &rec->inpj, &rec->inpk, &rec->inpl, &rec->inpm, &rec->inpn, &rec->inpo, &rec->inpp};
    try {
        if (rec->nmax < 1 || rec->nmax > 1048576) return S_db_badField;
        numeric::Registry registry; numeric::registerBuiltins(registry);
        const numeric::Metadata* m = registry.find(rec->op);
        if (!m || !m->argumentSchemaComplete || !m->arguments.empty() || m->stateful || m->inputs.empty() || m->inputs.size() > 16 || m->outputs.size() != 1 ||
            m->outputs[0].type != numeric::Type::Float64 || m->outputs[0].shape != numeric::Shape::Scalar) return S_db_badField;
        const bool legacy = rec->inp.type != CONSTANT;
        if (legacy && m->inputs.size() != 1) return S_db_badField;
        for (std::size_t i = 0; i < 16; ++i) {
            const bool configured = namedLinks[i]->type != CONSTANT;
            if ((legacy && configured) || (!legacy && configured != (i < m->inputs.size()))) return S_db_badField;
        }
        std::unique_ptr<State> state(new State);
        state->buffers.resize(m->inputs.size()); state->inputs.resize(m->inputs.size());
        for (std::size_t i = 0; i < m->inputs.size(); ++i) {
            const numeric::Descriptor& desc = m->inputs[i];
            if (desc.type != numeric::Type::Float64 || (desc.shape != numeric::Shape::Array && desc.shape != numeric::Shape::Scalar)) return S_db_badField;
            state->buffers[i].resize(desc.shape == numeric::Shape::Scalar ? 2 : static_cast<std::size_t>(rec->nmax) + 1);
            state->links.push_back(legacy ? &rec->inp : namedLinks[i]);
            state->shapes.push_back(desc.shape);
        }
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
    const char* argFields[] = {rec->arga, rec->argb, rec->argc, rec->argd, rec->arge, rec->argf, rec->argg, rec->argh,
                               rec->argi, rec->argj, rec->argk, rec->argl, rec->argm, rec->argn, rec->argo, rec->argp};
    bool argumentsEmpty = true;
    for (const char* field : argFields) if (field[0]) argumentsEmpty = false;
    bool ok = false;
    epicsUInt32 oldCount = rec->nord;
    rec->nord = 0;
    try {
        bool inputsValid = state && argumentsEmpty;
        std::size_t firstCount = 0;
        if (inputsValid) for (std::size_t i = 0; i < state->inputs.size(); ++i) {
            long count = static_cast<long>(state->buffers[i].size());
            if (dbGetLinkDBFtype(state->links[i]) != DBF_DOUBLE ||
                dbGetLink(state->links[i], DBR_DOUBLE, state->buffers[i].data(), nullptr, &count) != 0 ||
                count <= 0 || static_cast<unsigned long>(count) > rec->nmax ||
                (state->shapes[i] == numeric::Shape::Scalar && count != 1)) {
                inputsValid = false; break;
            }
            if (i == 0) firstCount = static_cast<std::size_t>(count);
            state->inputs[i] = {state->buffers[i].data(), static_cast<std::size_t>(count), state->buffers[i].size(), 1,
                                numeric::Type::Float64, state->shapes[i]};
        }
        if (inputsValid) {
            double value = 0;
            numeric::Output output{&value, 1, 0, numeric::Type::Float64, numeric::Shape::Scalar};
            if (state->op->execute(state->inputs.data(), state->inputs.size(), &output, 1).status == numeric::Status::Ok) {
                rec->val = value; rec->nord = static_cast<epicsUInt32>(firstCount); rec->udf = FALSE; ok = true;
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

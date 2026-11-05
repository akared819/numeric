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
#include <array>
#include <cmath>
#include <memory>
#include <vector>

namespace {
struct State {
    std::vector<std::vector<double>> buffers;
    std::vector<double> staging, output;
    std::vector<DBLINK*> links;
    std::vector<std::size_t> dataIndices;
    std::unique_ptr<numeric::Operation> op;
    numeric::Metadata metadata;
    std::vector<numeric::Input> inputs;
    std::vector<numeric::ArgumentValue> arguments;
    std::vector<double> slots;
    std::vector<const double*> legacyFields;
    std::array<std::array<char, 40>, 16> parsedText{};
    std::array<numeric::ArgumentValue, 16> parsedValues{};
    std::array<bool, 16> parsedValid{};
};
void destroyState(void* arg) { delete static_cast<State*>(arg); }
const double* legacyField(numericArrayRecord* rec, const std::string& name) {
    if (name == "window") return &rec->win;
    if (name == "alpha") return &rec->alph;
    if (name == "lower") return &rec->hmin;
    if (name == "upper") return &rec->hmax;
    if (name == "bins") return &rec->nbin;
    return nullptr;
}
long initRecord(dbCommon* common, int pass) {
    if (pass != 0) return 0;
    numericArrayRecord* rec = reinterpret_cast<numericArrayRecord*>(common);
    DBLINK* namedLinks[] = {&rec->inpa, &rec->inpb, &rec->inpc, &rec->inpd, &rec->inpe, &rec->inpf, &rec->inpg, &rec->inph,
                            &rec->inpi, &rec->inpj, &rec->inpk, &rec->inpl, &rec->inpm, &rec->inpn, &rec->inpo, &rec->inpp};
    try {
        if (rec->nmax < 1 || rec->nmax > 1048576 || rec->omax < 1 || rec->omax > 1048576) return S_db_badField;
        numeric::Registry registry; numeric::registerBuiltins(registry);
        const numeric::Metadata* m = registry.find(rec->op);
        if (!m || !m->argumentSchemaComplete || m->arguments.size() > 16 || m->stateful || m->inputs.empty() ||
            m->outputs.size() != 1 || m->outputs[0].type != numeric::Type::Float64 ||
            m->outputs[0].shape != numeric::Shape::Array) return S_db_badField;
        std::unique_ptr<State> state(new State);
        state->metadata = *m;
        state->inputs.resize(m->inputs.size());
        state->arguments.resize(m->arguments.size()); state->slots.resize(m->arguments.size());
        for (const auto& argument : m->arguments) {
            const double* field = legacyField(rec, argument.name);
            if (!field || argument.inputIndex >= m->inputs.size()) return S_db_badField;
            state->legacyFields.push_back(field);
        }
        for (std::size_t i = 0; i < m->inputs.size(); ++i) {
            bool parameter = false;
            for (const auto& argument : m->arguments) if (argument.inputIndex == i) parameter = true;
            if (parameter) continue;
            const numeric::Descriptor& desc = m->inputs[i];
            if (desc.type != numeric::Type::Float64 || (desc.shape != numeric::Shape::Array && desc.shape != numeric::Shape::Scalar)) return S_db_badField;
            state->dataIndices.push_back(i);
        }
        if (state->dataIndices.empty() || state->dataIndices.size() > 16) return S_db_badField;
        const bool legacy = rec->inp.type != CONSTANT;
        if (legacy && state->dataIndices.size() != 1) return S_db_badField;
        for (std::size_t i = 0; i < 16; ++i) {
            const bool configured = namedLinks[i]->type != CONSTANT;
            if ((legacy && configured) || (!legacy && configured != (i < state->dataIndices.size()))) return S_db_badField;
        }
        state->buffers.resize(state->dataIndices.size());
        for (std::size_t i = 0; i < state->dataIndices.size(); ++i) {
            const numeric::Shape shape = m->inputs[state->dataIndices[i]].shape;
            state->buffers[i].resize(shape == numeric::Shape::Scalar ? 2 : static_cast<std::size_t>(rec->nmax) + 1);
            state->links.push_back(legacy ? &rec->inp : namedLinks[i]);
        }
        state->staging.resize(rec->omax); state->output.resize(rec->omax);
        state->op = registry.create(rec->op);
        if (!state->op || epicsAtExit(destroyState, state.get())) return S_rec_outMem;
        rec->rpvt = state.release();
        return 0;
    } catch (...) { return S_rec_outMem; }
}
long process(dbCommon* common) {
    numericArrayRecord* rec = reinterpret_cast<numericArrayRecord*>(common);
    rec->pact = TRUE;
    State* state = static_cast<State*>(rec->rpvt);
    const char* argFields[] = {rec->arga, rec->argb, rec->argc, rec->argd, rec->arge, rec->argf, rec->argg, rec->argh,
                               rec->argi, rec->argj, rec->argk, rec->argl, rec->argm, rec->argn, rec->argo, rec->argp};
    bool ok = false;
    const epicsUInt32 oldInput = rec->nin, oldOutput = rec->nord;
    rec->nin = 0;
    try {
        bool inputsValid = state != nullptr;
        std::size_t firstCount = 0;
        if (inputsValid) for (std::size_t i = 0; i < state->dataIndices.size(); ++i) {
            const std::size_t index = state->dataIndices[i];
            const numeric::Shape shape = state->metadata.inputs[index].shape;
            long count = static_cast<long>(state->buffers[i].size());
            if (dbGetLinkDBFtype(state->links[i]) != DBF_DOUBLE ||
                dbGetLink(state->links[i], DBR_DOUBLE, state->buffers[i].data(), nullptr, &count) != 0 ||
                count <= 0 || static_cast<unsigned long>(count) > rec->nmax ||
                (shape == numeric::Shape::Scalar && count != 1)) {
                inputsValid = false; break;
            }
            if (i == 0) firstCount = static_cast<std::size_t>(count);
            state->inputs[index] = {state->buffers[i].data(), static_cast<std::size_t>(count), state->buffers[i].size(), 1,
                                    numeric::Type::Float64, shape};
        }
        if (inputsValid) {
            bool parametersValid = true;
            for (std::size_t i = state->arguments.size(); i < 16; ++i)
                if (argFields[i][0]) parametersValid = false;
            for (std::size_t i = 0; i < state->arguments.size(); ++i) {
                if (argFields[i][0]) {
                    if (!state->parsedValid[i] || std::strcmp(state->parsedText[i].data(), argFields[i]) != 0) {
                        const std::size_t length = std::strlen(argFields[i]);
                        numeric::ArgumentValue parsed{};
                        if (numeric::parseArgument(state->metadata.arguments[i].type, argFields[i], length, parsed).status != numeric::Status::Ok) {
                            parametersValid = false; break;
                        }
                        std::memcpy(state->parsedText[i].data(), argFields[i], length + 1);
                        state->parsedValues[i] = parsed; state->parsedValid[i] = true;
                    }
                    state->arguments[i] = state->parsedValues[i];
                    continue;
                }
                const double value = *state->legacyFields[i];
                const numeric::ArgumentType type = state->metadata.arguments[i].type;
                if (!std::isfinite(value)) { parametersValid = false; break; }
                if (type == numeric::ArgumentType::Floating) state->arguments[i] = {type, value, 0};
                else if (std::floor(value) == value && value >= -9223372036854775808.0 && value < 9223372036854775808.0)
                    state->arguments[i] = {type, 0.0, static_cast<std::int64_t>(value)};
                else { parametersValid = false; break; }
            }
            numeric::Output output{state->staging.data(), state->staging.size(), 0, numeric::Type::Float64, numeric::Shape::Array};
            if (parametersValid && numeric::setLegacyArguments(state->metadata, state->arguments.data(), state->arguments.size(),
                    state->inputs.data(), state->inputs.size(), state->slots.data(), state->slots.size()).status == numeric::Status::Ok &&
                state->op->execute(state->inputs.data(), state->inputs.size(), &output, 1).status == numeric::Status::Ok) {
                for (std::size_t i = 0; i < output.count; ++i) state->output[i] = state->staging[i];
                rec->nin = static_cast<epicsUInt32>(firstCount); rec->nord = static_cast<epicsUInt32>(output.count);
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

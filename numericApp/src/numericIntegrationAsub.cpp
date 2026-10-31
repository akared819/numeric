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
std::mutex instanceMutex;
std::map<aSubRecord*, std::unique_ptr<numeric::Operation>> instances;
void release(aSubRecord* rec) { std::lock_guard<std::mutex> lock(instanceMutex); instances.erase(rec); }
bool scalarLink(const DBLINK& link) {
    if (link.type == CONSTANT) return true;
    long count=0;
    return dbGetLinkDBFtype(&link)==DBF_DOUBLE && !dbGetNelements(&link,&count) && count==1;
}
}
extern "C" long numericPolynomialIntegral(aSubRecord* rec) {
    if (!rec) return -1;
    rec->neva=0;
    try {
        if (!numericIocReady() || rec->fta!=DBF_DOUBLE || rec->ftb!=DBF_DOUBLE || rec->ftc!=DBF_DOUBLE ||
            rec->ftva!=DBF_DOUBLE || !rec->a || !rec->b || !rec->c || !rec->vala || rec->nova<1 ||
            rec->inpa.type==CONSTANT || dbGetLinkDBFtype(&rec->inpa)!=DBF_DOUBLE || rec->nea>=rec->noa ||
            rec->nob!=1 || rec->neb!=1 || !scalarLink(rec->inpb) || rec->noc!=1 || rec->nec!=1 || !scalarLink(rec->inpc)) throw 1;
        if (rec->outa.type!=CONSTANT) {
            long capacity=0;
            if (dbGetLinkDBFtype(&rec->outa)!=DBF_DOUBLE || dbGetNelements(&rec->outa,&capacity) || capacity!=1) throw 1;
        }
        numeric::Operation* op;
        {
            std::lock_guard<std::mutex> lock(instanceMutex);
            auto it=instances.find(rec);
            if (it==instances.end()) {
                numeric::Registry registry; numeric::registerBuiltins(registry);
                auto fresh=registry.create("polynomial_integral");
                if (!fresh) throw 1;
                it=instances.emplace(rec,std::move(fresh)).first; rec->cadr=&release;
            }
            op=it->second.get();
        }
        numeric::Input input[] = {
            {static_cast<const double*>(rec->a),rec->nea,rec->noa,1,numeric::Type::Float64,numeric::Shape::Array},
            {static_cast<const double*>(rec->b),1,1,1,numeric::Type::Float64,numeric::Shape::Scalar},
            {static_cast<const double*>(rec->c),1,1,1,numeric::Type::Float64,numeric::Shape::Scalar}};
        numeric::Output output{static_cast<double*>(rec->vala),rec->nova,0,numeric::Type::Float64,numeric::Shape::Scalar};
        if (op->execute(input,3,&output,1).status!=numeric::Status::Ok) throw 1;
        rec->neva=output.count; rec->udf=FALSE; return 0;
    } catch (...) { recGblSetSevr(rec,CALC_ALARM,INVALID_ALARM); return -1; }
}
epicsRegisterFunction(numericPolynomialIntegral);

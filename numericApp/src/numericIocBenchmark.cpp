// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbStaticLib.h>
#include <epicsExit.h>
#include <iocInit.h>
#include <iocsh.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_version.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <exception>
#include <vector>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
namespace {
bool positive(const char* text, std::size_t maximum, std::size_t& value) {
    if (!text || !*text) return false;
    std::size_t n=0;
    for (const char* p=text; *p; ++p) {
        if (*p<'0' || *p>'9') return false;
        const unsigned digit=static_cast<unsigned>(*p-'0');
        if (n>(maximum-digit)/10) return false;
        n=n*10+digit;
    }
    if (!n || n>maximum) return false;
    value=n; return true;
}
}
int main(int argc,char** argv) {
    std::size_t length=0, iterations=0;
    if (argc!=4 || !positive(argv[1],1048576,length) || !positive(argv[2],10000000,iterations) || !*argv[3]) {
        std::fprintf(stderr,"usage: %s ARRAY_LENGTH ITERATIONS HARDWARE_DESCRIPTION\n",argv[0]); return 2;
    }
    try {
        std::vector<double> data(length);
        for (std::size_t i=0;i<length;++i) data[i]=static_cast<double>(i%101)-50.0;
        char macro[64]; std::snprintf(macro,sizeof(macro),"N=%zu",length);
        if (dbReadDatabase(&pdbbase,"dbd/numericIocTest.dbd",nullptr,nullptr) ||
            numericIocTest_registerRecordDeviceDriver(pdbbase) ||
            dbLoadRecords("numericApp/src/numericIocBenchmark.db",macro) || iocshCmd("numericInitialize") ||
            iocBuildIsolated() || iocRun()) {
            std::fprintf(stderr,"IOC setup failed\n"); return 1;
        }
        DBADDR input, process, result, severity, nord;
        if (dbNameToAddr("benchInput",&input) || dbNameToAddr("benchMean.PROC",&process) ||
            dbNameToAddr("benchMean.VAL",&result) || dbNameToAddr("benchMean.SEVR",&severity) || dbNameToAddr("benchMean.NORD",&nord) ||
            dbPutField(&input,DBR_DOUBLE,data.data(),static_cast<long>(length))) return 1;
        const epicsInt32 one=1;
        for (int i=0;i<10;++i) if (dbPutField(&process,DBR_LONG,&one,1)) return 1;
        using Clock=std::chrono::steady_clock;
        const auto start=Clock::now();
        for (std::size_t i=0;i<iterations;++i) if (dbPutField(&process,DBR_LONG,&one,1)) return 1;
        const auto end=Clock::now();
        double value=0; epicsUInt16 alarm=0; long count=1;
        if (dbGetField(&result,DBR_DOUBLE,&value,nullptr,&count,nullptr) || count!=1) return 1;
        count=1;
        epicsUInt32 used=0;
        if (dbGetField(&nord,DBR_ULONG,&used,nullptr,&count,nullptr) || count!=1 || used!=length) return 1;
        count=1;
        if (dbGetField(&severity,DBR_USHORT,&alarm,nullptr,&count,nullptr) || count!=1 || alarm ||
            value!=gsl_stats_mean(data.data(),1,length)) return 1;
        const double ns=std::chrono::duration<double,std::nano>(end-start).count()/iterations;
        std::printf("hardware=%s\ncompiler=%s\ngsl=%s\nalgorithm=mean\ntype=DOUBLE\narray_length=%zu\niterations=%zu\n",argv[3],__VERSION__,gsl_version,length,iterations);
        std::printf("ioc_mode=isolated_test_ioc\nrecord=numericScalar\ninput_link=local_waveform\nepics_record_and_link_overhead=yes\nnetwork_overhead=no\nsetup_and_input_write_included=no\ntimed_allocations=not_instrumented\n");
        std::printf("ioc_process_ns_per_call=%.1f\nresult=%.17g\n",ns,value);
        if (iocShutdown()) return 1;
        dbFreeBase(pdbbase); pdbbase=nullptr; epicsExitCallAtExits();
        return 0;
    } catch (const std::exception& e) { std::fprintf(stderr,"benchmark failed: %s\n",e.what()); return 1; }
}

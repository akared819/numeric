// SPDX-License-Identifier: GPL-3.0-or-later
#include "numeric.h"
#include "numericCompiler.h"
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_version.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <limits>
#include <vector>

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
    if (argc!=4 || !positive(argv[1],16777216,length) || !positive(argv[2],10000000,iterations) || !*argv[3]) {
        std::fprintf(stderr,"usage: %s ARRAY_LENGTH ITERATIONS HARDWARE_DESCRIPTION\n",argv[0]); return 2;
    }
    try {
        std::vector<double> data(length);
        for (std::size_t i=0;i<length;++i) data[i]=static_cast<double>(i%101)-50.0;
        numeric::initializeGsl(); numeric::Registry registry; numeric::registerBuiltins(registry);
        auto operation=registry.create("mean"); if (!operation) return 1;
        double value=0; numeric::Input in{data.data(),length,length,1,numeric::Type::Float64,numeric::Shape::Array};
        numeric::Output out{&value,1,0,numeric::Type::Float64,numeric::Shape::Scalar};
        const double reference=gsl_stats_mean(data.data(),1,length);
        for (int i=0;i<10;++i) {
            if (operation->execute(&in,1,&out,1).status!=numeric::Status::Ok || out.count!=1 || value!=reference) return 1;
        }
        volatile double sink=0;
        using Clock=std::chrono::steady_clock;
        auto start=Clock::now();
        for (std::size_t i=0;i<iterations;++i) sink+=gsl_stats_mean(data.data(),1,length);
        auto middle=Clock::now();
        for (std::size_t i=0;i<iterations;++i) {
            if (operation->execute(&in,1,&out,1).status!=numeric::Status::Ok) return 1;
            sink+=value;
        }
        auto end=Clock::now();
        if (!std::isfinite(sink) || value!=reference || out.count!=1) return 1;
        const double gslNs=std::chrono::duration<double,std::nano>(middle-start).count()/iterations;
        const double numericNs=std::chrono::duration<double,std::nano>(end-middle).count()/iterations;
        std::printf("hardware=%s\ncompiler=%s\ngsl=%s\nalgorithm=mean\ntype=DOUBLE\narray_length=%zu\niterations=%zu\n",argv[3],NUMERIC_COMPILER_VERSION,gsl_version,length,iterations);
        std::printf("timed_allocations=none\nepics_ioc_or_network_overhead=no\nvalidation=numeric_only\n");
        std::printf("direct_gsl_ns_per_call=%.1f\nnumeric_core_ns_per_call=%.1f\nchecksum=%.17g\n",gslNs,numericNs,static_cast<double>(sink));
        return 0;
    } catch (const std::exception& e) { std::fprintf(stderr,"benchmark failed: %s\n",e.what()); return 1; }
}

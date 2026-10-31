// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cmath>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
static bool matches(const char* name, const double* expected, long count) {
    DBADDR addr; double values[8] = {}; long actual = 8;
    if (dbNameToAddr(name, &addr) || dbGetField(&addr, DBR_DOUBLE, values, nullptr, &actual, nullptr) || actual != count) return false;
    for (long i=0; i<count; ++i) if (!std::isfinite(values[i]) || std::fabs(values[i]-expected[i]) > 1e-12) return false;
    return true;
}
MAIN(numericFftIocTest) {
    const bool noInit = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0);
    testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/fft.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericFftIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    double impulse[] = {1,0,0,0,0,0,0,0};
    double ones[] = {1,0,1,0,1,0,1,0};
    testdbPutArrFieldOk("fftInput", DBF_DOUBLE, 8, impulse);
    testdbPutFieldOk("fftForward.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("fftForward.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("fftForward.NEVA", DBF_ULONG, 0u);
        testdbGetFieldEqual("fftForward.SEVR", DBF_STRING, "INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("fftForward.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("fftForward.NEVA", DBF_ULONG, 8u);
    testOk(matches("fftOutput", ones, 8), "impulse produces constant spectrum");
    testdbPutFieldOk("fftInverse.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fftInverse.VAL", DBF_LONG, 0);
    testOk(matches("ifftOutput", impulse, 8), "inverse reconstructs input");
    testdbPutFieldOk("smallOutput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallOutput.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("smallOutput.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("smallOutput.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("smallLink.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallLink.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("smallLink.SEVR", DBF_STRING, "INVALID");
    testdbPutFieldOk("badOutputType.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("badOutputType.VAL", DBF_LONG, -1);
    testdbPutFieldOk("smallInput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("smallInput.VAL", DBF_LONG, -1);
    testdbPutFieldOk("badType.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("badType.VAL", DBF_LONG, -1);
    testdbPutFieldOk("constantInput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("constantInput.VAL", DBF_LONG, -1);
    testdbPutArrFieldOk("fftInput", DBF_DOUBLE, 3, impulse);
    testdbPutFieldOk("fftForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fftForward.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("fftForward.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("fftForward.SEVR", DBF_STRING, "INVALID");
    testOk(matches("fftOutput", ones, 8), "invalid input preserves output waveform");
    double invalid[] = {std::numeric_limits<double>::quiet_NaN(),0};
    testdbPutArrFieldOk("fftInput", DBF_DOUBLE, 2, invalid);
    testdbPutFieldOk("fftForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fftForward.VAL", DBF_LONG, -1);
    testOk(matches("fftOutput", ones, 8), "NaN preserves output waveform");
    invalid[0] = std::numeric_limits<double>::infinity();
    testdbPutArrFieldOk("fftInput", DBF_DOUBLE, 2, invalid);
    testdbPutFieldOk("fftForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fftForward.VAL", DBF_LONG, -1);
    testOk(matches("fftOutput", ones, 8), "infinity preserves output waveform");
    double huge[] = {std::numeric_limits<double>::max(),0,std::numeric_limits<double>::max(),0};
    testdbPutArrFieldOk("fftInput", DBF_DOUBLE, 4, huge);
    testdbPutFieldOk("fftForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fftForward.VAL", DBF_LONG, -1);
    testOk(matches("fftOutput", ones, 8), "numerical overflow preserves output waveform");
    testdbPutArrFieldOk("fftInput", DBF_DOUBLE, 8, impulse);
    testdbPutFieldOk("fftForward.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("fftForward.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("fftForward.SEVR", DBF_STRING, "NO_ALARM");
    testOk(matches("fftOutput", ones, 8), "valid input recovers after failure");
    testIocShutdownOk(); testdbCleanup(); return testDone();
}

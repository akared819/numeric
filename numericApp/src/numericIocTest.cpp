// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <limits>
#include <cstdlib>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
MAIN(numericIocTest) {
    const bool uninitialized = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(uninitialized ? 4 : 30);
    testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("numericApp/src/numericIocTest.db", nullptr, nullptr);
    if (!uninitialized) iocshCmd("numericInitialize");
    testIocInitOk();
    if (uninitialized) {
        testdbPutFieldOk("calc.PROC", DBF_LONG, 1);
        testdbGetFieldEqual("calc.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("calc.NEVA", DBF_ULONG, 0u);
        testdbGetFieldEqual("calc.SEVR", DBF_STRING, "INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    double values[] = {1, 3, 5};
    testdbPutArrFieldOk("input", DBF_DOUBLE, 3, values);
    testdbGetFieldEqual("result", DBF_DOUBLE, 0.0);
    testdbPutFieldOk("calc.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("calc.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("calc.NEA", DBF_ULONG, 3u);
    testdbGetFieldEqual("calc.NEVA", DBF_ULONG, 1u);
    testdbGetFieldEqual("result", DBF_DOUBLE, 3.0);
    testdbPutFieldOk("small.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("small.NEA", DBF_ULONG, 2u);
    testdbGetFieldEqual("small.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("small.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("small.SEVR", DBF_STRING, "INVALID");
    testdbPutArrFieldOk("input", DBF_DOUBLE, 2, values);
    testdbPutFieldOk("small.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("small.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("small.NEVA", DBF_ULONG, 0u);
    values[0] = std::numeric_limits<double>::quiet_NaN();
    testdbPutArrFieldOk("input", DBF_DOUBLE, 1, values);
    testdbPutFieldOk("calc.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("calc.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("calc.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("calc.STAT", DBF_STRING, "CALC");
    testdbGetFieldEqual("calc.SEVR", DBF_STRING, "INVALID");
    testdbGetFieldEqual("result", DBF_DOUBLE, 3.0);
    values[0] = 7;
    testdbPutArrFieldOk("input", DBF_DOUBLE, 1, values);
    testdbPutFieldOk("calc.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("calc.NEA", DBF_ULONG, 1u);
    testdbGetFieldEqual("result", DBF_DOUBLE, 7.0);
    testdbPutFieldOk("bad.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("bad.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("bad.SEVR", DBF_STRING, "INVALID");
    testIocShutdownOk();
    testdbCleanup();
    return testDone();
}

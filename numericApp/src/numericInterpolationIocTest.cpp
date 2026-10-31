// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
MAIN(numericInterpolationIocTest) {
    const bool noInit = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0); testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/interpolation.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericInterpolationIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    double x[] = {0,1,3}, y[] = {0,2,6};
    testdbPutArrFieldOk("interpX", DBF_DOUBLE, 3, x);
    testdbPutArrFieldOk("interpY", DBF_DOUBLE, 3, y);
    testdbPutFieldOk("interpEval.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("interpEval.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("interpEval.SEVR", DBF_STRING, "INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("interpEval.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("interpResult", DBF_DOUBLE, 4.0);
    testdbPutFieldOk("interpSmall.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("interpSmall.VAL", DBF_LONG, -1);
    testdbPutFieldOk("interpBadType.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("interpBadType.VAL", DBF_LONG, -1);
    testdbPutFieldOk("interpQuery", DBF_DOUBLE, 4.0);
    testdbPutFieldOk("interpEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("interpEval.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("interpEval.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("interpEval.SEVR", DBF_STRING, "INVALID");
    testdbGetFieldEqual("interpResult", DBF_DOUBLE, 4.0);
    testdbPutFieldOk("interpQuery", DBF_DOUBLE, 1.0);
    x[1] = 0; testdbPutArrFieldOk("interpX", DBF_DOUBLE, 3, x);
    testdbPutFieldOk("interpEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("interpEval.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("interpResult", DBF_DOUBLE, 4.0);
    x[1] = 1; testdbPutArrFieldOk("interpX", DBF_DOUBLE, 3, x);
    testdbPutArrFieldOk("interpY", DBF_DOUBLE, 2, y);
    testdbPutFieldOk("interpEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("interpEval.VAL", DBF_LONG, -1);
    testdbPutArrFieldOk("interpY", DBF_DOUBLE, 3, y);
    testdbPutFieldOk("interpEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("interpEval.SEVR", DBF_STRING, "NO_ALARM");
    testdbGetFieldEqual("interpResult", DBF_DOUBLE, 2.0);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}

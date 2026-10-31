// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
MAIN(numericPolynomialIocTest) {
    const bool noInit = std::getenv("NUMERIC_TEST_NO_INIT") != nullptr;
    testPlan(0); testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/polynomial.db", nullptr, "P=");
    testdbReadDatabase("numericApp/src/numericPolynomialIocTest.db", nullptr, nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    double coefficients[] = {2,3,4,5};
    testdbPutArrFieldOk("polyCoefficients", DBF_DOUBLE, 3, coefficients);
    testdbPutFieldOk("polyEval.PROC", DBF_LONG, 1);
    if (noInit) {
        testdbGetFieldEqual("polyEval.VAL", DBF_LONG, -1);
        testdbGetFieldEqual("polyEval.NEVA", DBF_ULONG, 0u);
        testdbGetFieldEqual("polyEval.SEVR", DBF_STRING, "INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("polyEval.VAL", DBF_LONG, 0);
    testdbGetFieldEqual("polyEval.NEVA", DBF_ULONG, 1u);
    testdbGetFieldEqual("polyResult", DBF_DOUBLE, 24.0);
    testdbPutFieldOk("polySmall.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polySmall.VAL", DBF_LONG, -1);
    testdbPutFieldOk("polyBadType.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyBadType.VAL", DBF_LONG, -1);
    testdbPutFieldOk("polyBadScalar.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyBadScalar.VAL", DBF_LONG, -1);
    epicsInt32 longCoefficients[] = {2,3,4};
    testdbPutArrFieldOk("polyLongCoefficients", DBF_LONG, 3, longCoefficients);
    testdbPutFieldOk("polyLongInput.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyLongInput.VAL", DBF_LONG, -1);
    testdbPutFieldOk("polyLongScalar.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyLongScalar.VAL", DBF_LONG, -1);
    testdbPutFieldOk("polyLongTarget.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyLongTarget.VAL", DBF_LONG, -1);
    testdbPutArrFieldOk("polyCoefficients", DBF_DOUBLE, 0, coefficients);
    testdbPutFieldOk("polyEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyEval.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("polyEval.NEVA", DBF_ULONG, 0u);
    testdbGetFieldEqual("polyEval.SEVR", DBF_STRING, "INVALID");
    testdbGetFieldEqual("polyResult", DBF_DOUBLE, 24.0);
    coefficients[1] = std::numeric_limits<double>::quiet_NaN();
    testdbPutArrFieldOk("polyCoefficients", DBF_DOUBLE, 3, coefficients);
    testdbPutFieldOk("polyEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyEval.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("polyResult", DBF_DOUBLE, 24.0);
    coefficients[1] = 3;
    testdbPutArrFieldOk("polyCoefficients", DBF_DOUBLE, 3, coefficients);
    testdbPutFieldOk("polyPoint", DBF_DOUBLE, std::numeric_limits<double>::quiet_NaN());
    testdbPutFieldOk("polyEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyEval.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("polyResult", DBF_DOUBLE, 24.0);
    testdbPutFieldOk("polyPoint", DBF_DOUBLE, std::numeric_limits<double>::max());
    testdbPutFieldOk("polyEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyEval.VAL", DBF_LONG, -1);
    testdbGetFieldEqual("polyResult", DBF_DOUBLE, 24.0);
    testdbPutFieldOk("polyPoint", DBF_DOUBLE, 2.0);
    testdbPutArrFieldOk("polyCoefficients", DBF_DOUBLE, 4, coefficients);
    testdbPutFieldOk("polyEval.PROC", DBF_LONG, 1);
    testdbGetFieldEqual("polyEval.SEVR", DBF_STRING, "NO_ALARM");
    testdbGetFieldEqual("polyResult", DBF_DOUBLE, 64.0);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}

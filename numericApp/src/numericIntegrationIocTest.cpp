// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
MAIN(numericIntegrationIocTest) {
    const bool noInit=std::getenv("NUMERIC_TEST_NO_INIT")!=nullptr;
    testPlan(0); testdbPrepare(); testdbReadDatabase("dbd/numericIocTest.dbd",nullptr,nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/integration.db",nullptr,"P=");
    testdbReadDatabase("numericApp/src/numericIntegrationIocTest.db",nullptr,nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    double c[]={2,3}; testdbPutArrFieldOk("integralCoefficients",DBF_DOUBLE,2,c);
    testdbPutFieldOk("integralEval.PROC",DBF_LONG,1);
    if (noInit) {
        testdbGetFieldEqual("integralEval.VAL",DBF_LONG,-1);
        testdbGetFieldEqual("integralEval.SEVR",DBF_STRING,"INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("integralEval.VAL",DBF_LONG,0);
    testdbGetFieldEqual("integralResult",DBF_DOUBLE,10.0);
    testdbPutFieldOk("integralSmall.PROC",DBF_LONG,1);
    testdbGetFieldEqual("integralSmall.VAL",DBF_LONG,0);
    double c3[]={2,3,4}; testdbPutArrFieldOk("integralCoefficients",DBF_DOUBLE,3,c3);
    testdbPutFieldOk("integralSmall.PROC",DBF_LONG,1); testdbGetFieldEqual("integralSmall.VAL",DBF_LONG,-1);
    testdbPutFieldOk("integralBadType.PROC",DBF_LONG,1); testdbGetFieldEqual("integralBadType.VAL",DBF_LONG,-1);
    testdbPutFieldOk("integralUpper",DBF_DOUBLE,std::numeric_limits<double>::infinity());
    testdbPutFieldOk("integralEval.PROC",DBF_LONG,1); testdbGetFieldEqual("integralEval.VAL",DBF_LONG,-1);
    testdbGetFieldEqual("integralEval.NEVA",DBF_ULONG,0u); testdbGetFieldEqual("integralEval.SEVR",DBF_STRING,"INVALID");
    testdbGetFieldEqual("integralResult",DBF_DOUBLE,10.0);
    testdbPutArrFieldOk("integralCoefficients",DBF_DOUBLE,2,c);
    testdbPutFieldOk("integralUpper",DBF_DOUBLE,2.0); testdbPutFieldOk("integralEval.PROC",DBF_LONG,1);
    testdbGetFieldEqual("integralEval.SEVR",DBF_STRING,"NO_ALARM"); testdbGetFieldEqual("integralResult",DBF_DOUBLE,10.0);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}

// SPDX-License-Identifier: GPL-3.0-or-later
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <iocsh.h>
#include <testMain.h>
#include <cstdlib>
#include <limits>
extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
MAIN(numericRootsIocTest) {
    const bool noInit=std::getenv("NUMERIC_TEST_NO_INIT")!=nullptr;
    testPlan(0); testdbPrepare(); testdbReadDatabase("dbd/numericIocTest.dbd",nullptr,nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    testdbReadDatabase("examples/roots.db",nullptr,"P=");
    testdbReadDatabase("numericApp/src/numericRootsIocTest.db",nullptr,nullptr);
    if (!noInit) iocshCmd("numericInitialize");
    testIocInitOk();
    double c[]={-1,1}; testdbPutArrFieldOk("rootCoefficients",DBF_DOUBLE,2,c);
    testdbPutFieldOk("rootEval.PROC",DBF_LONG,1);
    if (noInit) {
        testdbGetFieldEqual("rootEval.VAL",DBF_LONG,-1);
        testdbGetFieldEqual("rootEval.SEVR",DBF_STRING,"INVALID");
        testIocShutdownOk(); testdbCleanup(); return testDone();
    }
    testdbGetFieldEqual("rootEval.VAL",DBF_LONG,0);
    testdbGetFieldEqual("rootResult",DBF_DOUBLE,1.0);
    testdbPutFieldOk("rootSmall.PROC",DBF_LONG,1);
    testdbGetFieldEqual("rootSmall.VAL",DBF_LONG,0);
    double c3[]={-1,1,0}; testdbPutArrFieldOk("rootCoefficients",DBF_DOUBLE,3,c3);
    testdbPutFieldOk("rootSmall.PROC",DBF_LONG,1); testdbGetFieldEqual("rootSmall.VAL",DBF_LONG,-1);
    testdbPutFieldOk("rootBadType.PROC",DBF_LONG,1); testdbGetFieldEqual("rootBadType.VAL",DBF_LONG,-1);
    testdbPutFieldOk("rootUpper",DBF_DOUBLE,0.5); testdbPutFieldOk("rootEval.PROC",DBF_LONG,1);
    testdbGetFieldEqual("rootEval.VAL",DBF_LONG,-1); testdbGetFieldEqual("rootResult",DBF_DOUBLE,1.0);
    testdbPutFieldOk("rootUpper",DBF_DOUBLE,std::numeric_limits<double>::infinity());
    testdbPutFieldOk("rootEval.PROC",DBF_LONG,1); testdbGetFieldEqual("rootEval.VAL",DBF_LONG,-1);
    testdbGetFieldEqual("rootEval.NEVA",DBF_ULONG,0u); testdbGetFieldEqual("rootEval.SEVR",DBF_STRING,"INVALID");
    testdbGetFieldEqual("rootResult",DBF_DOUBLE,1.0);
    testdbPutArrFieldOk("rootCoefficients",DBF_DOUBLE,2,c);
    testdbPutFieldOk("rootUpper",DBF_DOUBLE,2.0); testdbPutFieldOk("rootEval.PROC",DBF_LONG,1);
    testdbGetFieldEqual("rootEval.SEVR",DBF_STRING,"NO_ALARM"); testdbGetFieldEqual("rootResult",DBF_DOUBLE,1.0);
    testIocShutdownOk(); testdbCleanup(); return testDone();
}

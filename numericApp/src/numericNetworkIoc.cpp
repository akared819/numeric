// SPDX-License-Identifier: GPL-3.0-or-later
#include <aSubRecord.h>
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <epicsThread.h>
#include <iocInit.h>
#include <iocsh.h>
#include <registryFunction.h>
#include <testMain.h>

extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
static long delayEvent(aSubRecord*) {
    // Keep this synchronous: the event and consumer remain in one lock set.
    epicsThreadSleep(3.0);
    return 0;
}
MAIN(numericNetworkIoc) {
    testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    registryFunctionAdd("numericTestDelay", reinterpret_cast<REGISTRYFUNCTION>(delayEvent));
    testdbReadDatabase("numericApp/src/numericIocTest.db", nullptr, nullptr);
    testdbReadDatabase("examples/acknowledged-stats.db", nullptr, "P=demo:");
    testdbReadDatabase("numericApp/src/numericNetworkTest.db", nullptr, nullptr);
    iocshCmd("numericInitialize");
    if (iocInit()) return 1;
    for (;;) epicsThreadSleep(60.0);
}

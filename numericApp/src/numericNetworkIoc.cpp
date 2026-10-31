// SPDX-License-Identifier: GPL-3.0-or-later
#include <aSubRecord.h>
#include <dbAccess.h>
#include <dbUnitTest.h>
#include <epicsThread.h>
#include <iocInit.h>
#include <iocsh.h>
#include <registryFunction.h>
#include <testMain.h>
#include <cstdio>
#include <cstdlib>
#include <string>

extern "C" int numericIocTest_registerRecordDeviceDriver(struct dbBase*);
static long delayEvent(aSubRecord*) {
    // Keep this synchronous: the event and consumer remain in one lock set.
    const char* marker = std::getenv("NUMERIC_TEST_CRASH_MARKER");
    if (marker) {
        const std::string armed = std::string(marker) + ".arm";
        if (FILE* input = std::fopen(armed.c_str(), "r")) {
            std::fclose(input);
            if (FILE* output = std::fopen(marker, "w")) {
                std::fputs("entered\n", output);
                std::fclose(output);
            }
            epicsThreadSleep(30.0);
            return 0;
        }
    }
    epicsThreadSleep(3.0);
    return 0;
}
static long delayAfterUpdate(aSubRecord* record) {
    const char* marker = std::getenv("NUMERIC_TEST_AFTER_MARKER");
    if (!marker) return 0;
    const std::string armed = std::string(marker) + ".arm";
    FILE* input = std::fopen(armed.c_str(), "r");
    if (!input) return 0;
    std::fclose(input);
    const double* values = static_cast<const double*>(record->a);
    // Signal only after the consumer published the exact event and result.
    if (record->nea != 11 || !values || values[0] != 7 || values[6] != 1 ||
        values[7] != 901 || values[8] != 0 || values[9] != 0 || values[10] != 7) return -1;
    FILE* output = std::fopen(marker, "w");
    if (!output) return -1;
    std::fputs("updated\n", output);
    std::fclose(output);
    epicsThreadSleep(30.0);
    return 0;
}
MAIN(numericNetworkIoc) {
    testdbPrepare();
    testdbReadDatabase("dbd/numericIocTest.dbd", nullptr, nullptr);
    numericIocTest_registerRecordDeviceDriver(pdbbase);
    registryFunctionAdd("numericTestDelay", reinterpret_cast<REGISTRYFUNCTION>(delayEvent));
    registryFunctionAdd("numericTestAfterUpdate", reinterpret_cast<REGISTRYFUNCTION>(delayAfterUpdate));
    testdbReadDatabase("numericApp/src/numericIocTest.db", nullptr, nullptr);
    testdbReadDatabase("examples/acknowledged-stats.db", nullptr, "P=demo:");
    testdbReadDatabase("examples/acknowledged-stats.db", nullptr, "P=restart:");
    testdbReadDatabase("examples/acknowledged-stats.db", nullptr, "P=ambiguous:");
    testdbReadDatabase("numericApp/src/numericNetworkTest.db", nullptr, nullptr);
    iocshCmd("numericInitialize");
    if (iocInit()) return 1;
    for (;;) epicsThreadSleep(60.0);
}

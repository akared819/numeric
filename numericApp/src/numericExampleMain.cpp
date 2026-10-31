// SPDX-License-Identifier: GPL-3.0-or-later
#include <epicsExit.h>
#include <epicsThread.h>
#include <iocsh.h>
#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && std::strcmp(argv[2], "--headless"))) {
        std::fprintf(stderr, "Usage: numericExample startup-script [--headless]\n");
        return 2;
    }
    if (iocsh(argv[1])) return 1;
    if (argc == 3) {
        for (;;) epicsThreadSleep(60.0);
    }
    iocsh(nullptr);
    epicsExit(0);
    return 0;
}

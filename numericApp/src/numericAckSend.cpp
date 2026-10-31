// SPDX-License-Identifier: GPL-3.0-or-later
#include "numericAckClient.h"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

static bool integer(const char* text, std::uint64_t& value) {
    if (!*text || *text == '-') return false;
    char* end = nullptr; errno = 0;
    const unsigned long long parsed = std::strtoull(text, &end, 10);
    if (errno || *end) return false;
    value = parsed; return true;
}
int main(int argc, char** argv) {
    numeric::Event event{};
    if ((argc != 6 && argc != 7) || !integer(argv[3], event.session) || !integer(argv[4], event.sequence)) {
        std::cerr << "Usage: numericAckSend EVENT_PV RECORD SESSION SEQUENCE sample VALUE\n"
                     "   or: numericAckSend EVENT_PV RECORD SESSION SEQUENCE reset\n";
        return 1;
    }
    const std::string kind(argv[5]);
    if (kind == "reset" && argc == 6) { event.kind = numeric::EventKind::Reset; event.value = 0; }
    else if (kind == "sample" && argc == 7) {
        char* end = nullptr; errno = 0; event.value = std::strtod(argv[6], &end);
        if (errno || end == argv[6] || *end || !std::isfinite(event.value)) { std::cerr << "Invalid sample\n"; return 1; }
        event.kind = numeric::EventKind::Sample;
    } else { std::cerr << "Invalid event kind or arguments\n"; return 1; }
    const numeric::SendResult result = numeric::sendAcknowledged(argv[1], argv[2], event, 3, 5.0);
    if (result.status == numeric::SendStatus::Acknowledged) {
        std::cout << "Acknowledged session " << event.session << " sequence " << event.sequence << "\nStatistics:" << std::setprecision(17);
        for (double value : result.statistics) std::cout << ' ' << value;
        std::cout << '\n'; return 0;
    }
    if (result.status == numeric::SendStatus::Uncertain) {
        std::cerr << "Outcome uncertain: retry the identical event before advancing the sequence.\n"; return 2;
    }
    std::cerr << "Invalid arguments or CA context.\n";
    return 1;
}

// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_ACK_PARSE_H
#define NUMERIC_ACK_PARSE_H
#include <cerrno>
#include <cmath>
#include <cstdlib>

inline bool numericParseSample(const char* text, double& value) {
    char* end = nullptr;
    errno = 0;
    const double parsed = std::strtod(text, &end);
    if (end == text || *end || !std::isfinite(parsed) ||
        (errno && (errno != ERANGE || parsed == 0.0))) return false;
    value = parsed;
    return true;
}
#endif

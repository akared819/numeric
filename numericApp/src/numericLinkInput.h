// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_LINK_INPUT_H
#define NUMERIC_LINK_INPUT_H
#include "numeric.h"
#include <dbLink.h>
#include <epicsTypes.h>
#include <vector>

// One instance per record input. Temporary typed buffers are allocated on the
// first use of a type and retained until record cleanup.
class NumericLinkInput {
public:
    void configure(std::size_t capacity);
    numeric::Result read(DBLINK& link, numeric::Shape shape, std::size_t maximum,
                         numeric::Input& output);
private:
    std::vector<double> values_;
    std::vector<epicsFloat32> floats_;
    std::vector<epicsInt32> integers_;
    std::vector<epicsInt64> integers64_;
};
#endif

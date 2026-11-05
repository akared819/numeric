// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_TYPED_MEAN_H
#define NUMERIC_TYPED_MEAN_H
#include "numericData.h"
#include <vector>
namespace numeric {
// Per-instance workspace; callers must synchronize concurrent use of one instance.
class NUMERIC_API TypedMean {
public:
    explicit TypedMean(std::size_t maxElements);
    TypedMean(const TypedMean&) = delete;
    TypedMean& operator=(const TypedMean&) = delete;
    Result reserve();
    Result execute(const ArrayView& input, double& output);
    void reset();
private:
    std::size_t maxElements_;
    std::vector<double> workspace_;
};
}
#endif

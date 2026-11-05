// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_DATA_INTERNAL_H
#define NUMERIC_DATA_INTERNAL_H
#include "numericData.h"
namespace numeric { namespace detail {
// Caller must validate the view and provide an in-bounds byte offset.
Result readValidatedElement(const ArrayView& view, std::size_t offset, ElementValue& value);
} }
#endif

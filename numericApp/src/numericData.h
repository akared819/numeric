// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_DATA_H
#define NUMERIC_DATA_H
#include "numeric.h"

namespace numeric {
// Complex values are two consecutive native-endian real components.
enum class ElementType { Float32, Float64, Int32, Int64, Complex64, Complex128 };
struct ArrayView {
    const void* data;
    std::size_t capacityBytes;
    ElementType type;
    std::size_t rank;
    const std::size_t* extents;
    const std::size_t* stridesBytes;
};
struct MutableArrayView {
    void* data;
    std::size_t capacityBytes;
    ElementType type;
    std::size_t rank;
    const std::size_t* extents;
    const std::size_t* stridesBytes;
};
// integer is used for integer types; real and imaginary for the others.
struct ElementValue { ElementType type; std::int64_t integer; double real; double imaginary; };
// Rank is limited to eight. Rank zero denotes one scalar; zero extent is empty.
NUMERIC_API Result validateArray(const ArrayView& view);
NUMERIC_API Result validateWritableArray(const MutableArrayView& view);
NUMERIC_API Result readElement(const ArrayView& view, const std::size_t* indices,
                               std::size_t indexCount, ElementValue& value);
// Exact conversion only; complex-to-real requires an exactly zero imaginary part.
// On failure, the output value is unchanged.
NUMERIC_API Result convertElement(const ElementValue& source, ElementType destination,
                                  ElementValue& converted);
// Same-type exact writes only; failed writes preserve the destination.
NUMERIC_API Result writeElement(const MutableArrayView& view, const std::size_t* indices,
                                std::size_t indexCount, const ElementValue& value);
}
#endif

// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_COMPILER_H
#define NUMERIC_COMPILER_H
#define NUMERIC_STRINGIFY_IMPL(value) #value
#define NUMERIC_STRINGIFY(value) NUMERIC_STRINGIFY_IMPL(value)
#if defined(__VERSION__)
#  define NUMERIC_COMPILER_VERSION __VERSION__
#elif defined(_MSC_FULL_VER)
#  define NUMERIC_COMPILER_VERSION "MSVC " NUMERIC_STRINGIFY(_MSC_FULL_VER)
#else
#  define NUMERIC_COMPILER_VERSION "unknown"
#endif
#endif

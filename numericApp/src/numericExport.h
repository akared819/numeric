// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef NUMERIC_EXPORT_H
#define NUMERIC_EXPORT_H

// EPICS sets BUILDING_numeric_API only for sources in the numeric library.
// Standalone core builds need no EPICS headers; static builds leave this empty.
#if defined(_WIN32) && defined(BUILDING_numeric_API) && defined(EPICS_BUILD_DLL)
#  define NUMERIC_API __declspec(dllexport)
#elif defined(_WIN32) && !defined(BUILDING_numeric_API) && defined(EPICS_CALL_DLL)
#  define NUMERIC_API __declspec(dllimport)
#else
#  define NUMERIC_API
#endif
#endif

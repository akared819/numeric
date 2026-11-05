/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional macOS benchmark probe; counts dynamically interposed allocation calls. */
#include <malloc/malloc.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>

static _Atomic int enabled;
static _Atomic unsigned long long calls;
void numericAllocationBegin(void) { atomic_store(&calls, 0); atomic_store(&enabled, 1); }
unsigned long long numericAllocationEnd(void) { atomic_store(&enabled, 0); return atomic_load(&calls); }
static void count(void) { if (atomic_load(&enabled)) atomic_fetch_add(&calls, 1); }
static void* measured_malloc(size_t size) { count(); return malloc_zone_malloc(malloc_default_zone(), size); }
static void* measured_calloc(size_t n, size_t size) { count(); return malloc_zone_calloc(malloc_default_zone(), n, size); }
static void* measured_realloc(void* ptr, size_t size) {
    count();
    malloc_zone_t* zone = ptr ? malloc_zone_from_ptr(ptr) : malloc_default_zone();
    return zone ? malloc_zone_realloc(zone, ptr, size) : NULL;
}
#define INTERPOSE(replacement, original) \
    __attribute__((used)) static const struct { const void* replacement_ptr; const void* original_ptr; } \
    interpose_##original __attribute__((section("__DATA,__interpose"))) = { (const void*)&replacement, (const void*)&original }
INTERPOSE(measured_malloc, malloc);
INTERPOSE(measured_calloc, calloc);
INTERPOSE(measured_realloc, realloc);

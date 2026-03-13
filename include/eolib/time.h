#ifndef EOLIB_TIME_H
#define EOLIB_TIME_H

#include <stdint.h>
#include <time.h>

/**
 * Emulation of Borland C++'s time() function as used by the original EO client.
 * Returns a 32-bit unsigned integer computed from local system time using the
 * same algorithm as the Borland runtime. The result is timezone-dependent —
 * approximately time(NULL) + local_timezone_offset_seconds + 18000 — which is
 * why a custom implementation is needed to reproduce the original client's RNG seeds.
 * Returns (uint32_t)-1 if the current time cannot be represented (year outside 1970–2038).
 */
uint32_t eo_time();

#endif
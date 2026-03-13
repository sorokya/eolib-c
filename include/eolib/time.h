#ifndef EOLIB_TIME_H
#define EOLIB_TIME_H

#include <stdint.h>
#include <time.h>

/**
 * Emulation of Borland C++'s time() function, which returns the number of seconds since
 * January 1, 1900, 00:00:00 UTC, adjusted to the local time zone. The return value is
 * a 32-bit unsigned integer, and the function returns -1 if the current time cannot
 * be represented in this format (e.g., if the year is before 1970 or after 2038).
 */
uint32_t eo_time();

#endif
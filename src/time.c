#include "eolib/time.h"

uint32_t eo_time()
{
    time_t now = time(NULL);
    struct tm local;
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif

    // tm_year is years since 1900 (standard C), used directly in the Borland formula
    int year = local.tm_year;
    int month = local.tm_mon;
    int day = local.tm_mday - 1;
    int hour = local.tm_hour;
    int min = local.tm_min;
    int sec = local.tm_sec;

    // days in month & days to month arrays
    static const int days_in_month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static const int days_to_month[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    if (year < 70 || year > 138)
        return -1;

    // carry seconds → minutes → hours → days
    int total_minutes = sec / 60 + min;
    int remaining_seconds = sec % 60;
    int total_hours = total_minutes / 60 + hour;
    int remaining_minutes = total_minutes % 60;
    int total_days = total_hours / 24 + day;
    int remaining_hours = total_hours % 24;

    int years = month / 12 + year;
    int months = month % 12;

    // month/day overflow loop
    while (total_days >= days_in_month[months])
    {
        if ((years & 3) != 0 || months != 1)
        {
            total_days -= days_in_month[months];
            months++;
        }
        else
        { // leap Feb
            if (total_days <= 28)
                break;
            total_days -= 29;
            months = 2;
        }
        if (months >= 12)
        {
            months -= 12;
            years++;
        }
    }

    unsigned int year_offset = years - 70;
    int leap_adj = year_offset + 2;
    if (leap_adj < 0)
        leap_adj = year_offset + 5;
    int leap_days = leap_adj >> 2;
    if (((year_offset + 70) & 3) == 0 && months < 2)
        leap_days--;

    int total_seconds = remaining_seconds +
                        60 * remaining_minutes +
                        3600 * remaining_hours +
                        86400 * (total_days + days_to_month[months] + 365 * year_offset + leap_days) + 18000; // +5 hours

    return total_seconds > 0 ? total_seconds : -1;
}
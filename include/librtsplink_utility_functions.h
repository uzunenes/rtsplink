#ifndef LIBFOIOT_UTILITY_FUNCTIONS_H
#define LIBFOIOT_UTILITY_FUNCTIONS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Suspends execution for a given number of milliseconds.
 * 
 * @param milisec Duration to sleep in milliseconds.
 */
void foiot_sleep_milisec(unsigned int milisec);

/**
 * @brief Returns the current system time in seconds since the Unix epoch (1970).
 * 
 * @return Current system time as a double (seconds + microseconds fraction).
 */
double foiot_get_system_time(void);

#ifdef __cplusplus
}
#endif

#endif // LIBFOIOT_UTILITY_FUNCTIONS_H


#include "../include/librtsplink_utility_functions.h"

#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <sys/time.h>
#include <cstdio>
#include <cstring>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <openssl/evp.h>
#include <openssl/err.h>

void foiot_sleep_milisec(unsigned int milisec)
{
    // max 1 saniyeyi aşan durumlarda bile güvenli uyku
    usleep(static_cast<useconds_t>(milisec) * 1000);
}

double foiot_get_system_time()
{
    struct timeval time {};
    
    if (gettimeofday(&time, nullptr) != 0)
    {
        perror("gettimeofday failed");
        return 0.0;
    }

    return static_cast<double>(time.tv_sec) + static_cast<double>(time.tv_usec) * 1e-6;
}


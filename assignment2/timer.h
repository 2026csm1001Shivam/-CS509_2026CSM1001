#ifndef TIMER_H
#define TIMER_H

#ifdef _WIN32
#include <windows.h>
typedef struct {
    LARGE_INTEGER freq;
    LARGE_INTEGER start;
} Stopwatch;
#else
#include <time.h>
typedef struct {
    struct timespec start;
} Stopwatch;
#endif

static inline void stopwatch_start(Stopwatch *s) {
#ifdef _WIN32
    QueryPerformanceFrequency(&s->freq);
    QueryPerformanceCounter(&s->start);
#else
    clock_gettime(CLOCK_MONOTONIC, &s->start);
#endif
}

static inline double stopwatch_ms(const Stopwatch *s) {
#ifdef _WIN32
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return 1000.0 * (double)(now.QuadPart - s->start.QuadPart) /
           (double)s->freq.QuadPart;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    double secs = (double)(now.tv_sec - s->start.tv_sec) +
                  (double)(now.tv_nsec - s->start.tv_nsec) / 1e9;
    return secs * 1000.0;
#endif
}

#endif
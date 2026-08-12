#ifndef FLOYD_H
#define FLOYD_H

#include <stdint.h>

void floyd_warshall(int64_t *D, int V, int *neg_cycle);

#endif
#ifndef BELLMAN_H
#define BELLMAN_H

#include "graph.h"

int bellman_ford(const CSRGraph *g, int src, int64_t *dist);

#endif
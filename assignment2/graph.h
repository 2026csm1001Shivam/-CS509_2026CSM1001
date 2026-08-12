#ifndef GRAPH_H
#define GRAPH_H

#include <stdint.h>
#include <stdio.h>

#define GRAPH_INF ((int64_t)1 << 60)

typedef struct {
    int V;
    int E;
    int directed;
    int weighted;
    int *deg;
    int **nb;
    int64_t **wt;
} AdjList;

typedef struct {
    int V;
    int E;
    int *row_ptr;
    int *col_idx;
    int64_t *values;
} CSRGraph;

int read_weighted_adjlist(const char *path, AdjList *g, int *src,
                          char *err, size_t errsz);

int read_dense_matrix(const char *path, int *V, int64_t **mat,
                      char *err, size_t errsz);

void AdjListToCSR(const AdjList *g, CSRGraph *csr);

void free_adjlist(AdjList *g);
void free_csr(CSRGraph *g);

void fprint_i64(FILE *f, int64_t v);

#endif
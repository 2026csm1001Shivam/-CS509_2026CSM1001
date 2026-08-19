#include "kruskal.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int u, v;
    int64_t w;
} KE;

static int ke_cmp(const void *a, const void *b) {
    const KE *x = (const KE *)a;
    const KE *y = (const KE *)b;
    if (x->w != y->w) return (x->w < y->w) ? -1 : 1;
    if (x->u != y->u) return x->u - y->u;
    return x->v - y->v;
}

typedef struct {
    int *p;
    int *r;
} DSU;

static int dsu_find(DSU *d, int x) {
    while (d->p[x] != x) {
        d->p[x] = d->p[d->p[x]];
        x = d->p[x];
    }
    return x;
}

static void dsu_union(DSU *d, int a, int b) {
    a = dsu_find(d, a);
    b = dsu_find(d, b);
    if (a == b) return;
    if (d->r[a] < d->r[b]) {
        int t = a;
        a = b;
        b = t;
    }
    d->p[b] = a;
    if (d->r[a] == d->r[b]) d->r[a]++;
}

int64_t kruskal_mst(const CSRGraph *g, MstEdge *out, int *n_out) {
    int V = g->V;
    int E = g->E / 2; /* each undirected edge stored twice in the CSR */
    int64_t total = 0;

    KE *edges = (KE *)malloc((size_t)E * sizeof(KE));
    if (!edges) {
        fprintf(stderr, "kruskal: out of memory\n");
        exit(1);
    }
    int n = 0;
    for (int u = 0; u < V; u++) {
        for (int e = g->row_ptr[u]; e < g->row_ptr[u + 1]; e++) {
            int v = g->col_idx[e];
            if (u < v) {
                edges[n].u = u;
                edges[n].v = v;
                edges[n].w = g->values[e];
                n++;
            }
        }
    }

    qsort(edges, (size_t)n, sizeof(KE), ke_cmp);

    DSU d;
    d.p = (int *)malloc((size_t)V * sizeof(int));
    d.r = (int *)malloc((size_t)V * sizeof(int));
    if (!d.p || !d.r) {
        fprintf(stderr, "kruskal: out of memory\n");
        exit(1);
    }
    for (int v = 0; v < V; v++) {
        d.p[v] = v;
        d.r[v] = 0;
    }

    int m = 0;
    for (int i = 0; i < n && m < V - 1; i++) {
        if (dsu_find(&d, edges[i].u) != dsu_find(&d, edges[i].v)) {
            dsu_union(&d, edges[i].u, edges[i].v);
            out[m].u = edges[i].u;
            out[m].v = edges[i].v;
            out[m].w = edges[i].w;
            total += edges[i].w;
            m++;
        }
    }

    free(d.p);
    free(d.r);
    free(edges);
    *n_out = m;
    return total;
}

#include "prim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int64_t key;
    int v;
} HNode;

typedef struct {
    HNode *a;
    int n;
    int cap;
} Heap;

static void heap_push(Heap *h, int64_t key, int v) {
    if (h->n == h->cap) {
        h->cap = h->cap ? h->cap * 2 : 64;
        h->a = (HNode *)realloc(h->a, (size_t)h->cap * sizeof(HNode));
        if (!h->a) {
            fprintf(stderr, "prim: out of memory\n");
            exit(1);
        }
    }
    int i = h->n++;
    h->a[i].key = key;
    h->a[i].v = v;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->a[p].key <= h->a[i].key) break;
        HNode t = h->a[p];
        h->a[p] = h->a[i];
        h->a[i] = t;
        i = p;
    }
}

static HNode heap_pop(Heap *h) {
    HNode top = h->a[0];
    h->a[0] = h->a[--h->n];
    int i = 0;
    for (;;) {
        int l = 2 * i + 1, r = 2 * i + 2, m = i;
        if (l < h->n && h->a[l].key < h->a[m].key) m = l;
        if (r < h->n && h->a[r].key < h->a[m].key) m = r;
        if (m == i) break;
        HNode t = h->a[m];
        h->a[m] = h->a[i];
        h->a[i] = t;
        i = m;
    }
    return top;
}

int64_t prim_mst(const CSRGraph *g, MstEdge *out, int *n_out) {
    int V = g->V;

    int64_t *dist = (int64_t *)malloc((size_t)V * sizeof(int64_t));
    int *parent = (int *)malloc((size_t)V * sizeof(int));
    char *in_tree = (char *)calloc((size_t)V, sizeof(char));
    if (!dist || !parent || !in_tree) {
        fprintf(stderr, "prim: out of memory\n");
        exit(1);
    }
    for (int v = 0; v < V; v++) dist[v] = GRAPH_INF;
    for (int v = 0; v < V; v++) parent[v] = -1;

    Heap heap;
    memset(&heap, 0, sizeof(heap));

    dist[0] = 0;
    heap_push(&heap, 0, 0);

    int64_t total = 0;
    int m = 0;
    int added = 0;

    while (added < V && heap.n > 0) {
        HNode t = heap_pop(&heap);
        int v = t.v;
        if (in_tree[v]) continue;
        in_tree[v] = 1;
        added++;
        if (parent[v] >= 0) {
            out[m].u = parent[v];
            out[m].v = v;
            out[m].w = t.key;
            total += t.key;
            m++;
        }
        for (int e = g->row_ptr[v]; e < g->row_ptr[v + 1]; e++) {
            int u = g->col_idx[e];
            int64_t w = g->values[e];
            if (!in_tree[u] && w < dist[u]) {
                dist[u] = w;
                parent[u] = v;
                heap_push(&heap, w, u);
            }
        }
    }

    free(dist);
    free(parent);
    free(in_tree);
    free(heap.a);

    *n_out = m;
    return total;
}

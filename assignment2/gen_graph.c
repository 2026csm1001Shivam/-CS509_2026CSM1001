#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "graph.h"

typedef struct {
    int u, v;
    int64_t w;
} GenEdge;

static uint64_t rng_state;
static uint64_t rnd64(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 7;
    rng_state ^= rng_state << 17;
    return rng_state;
}
static int rnd_int(int lo, int hi) {
    return (int)(lo + rnd64() % (uint64_t)(hi - lo + 1));
}

static int edge_cmp(const void *a, const void *b) {
    const GenEdge *x = (const GenEdge *)a;
    const GenEdge *y = (const GenEdge *)b;
    if (x->u != y->u) return x->u - y->u;
    return x->v - y->v;
}

static GenEdge *build_edges(int V, int E, uint64_t seed, int *nout) {
    rng_state = seed ? seed : 1;
    GenEdge *edges = (GenEdge *)malloc((size_t)E * sizeof(GenEdge));
    if (!edges) {
        fprintf(stderr, "gen: out of memory\n");
        exit(1);
    }
    int n = 0;
    for (int i = 0; i < V - 1; i++) {
        edges[n].u = i;
        edges[n].v = i + 1;
        edges[n].w = 1;
        n++;
    }
    int guard = 0;
    while (n < E && guard < E * 10 + 1000) {
        guard++;
        int u = rnd_int(0, V - 2);
        int v = rnd_int(u + 1, V - 1);
        int64_t w = (rnd64() % 100 < 20) ? -rnd_int(1, 500) : rnd_int(1, 1000);
        edges[n].u = u;
        edges[n].v = v;
        edges[n].w = w;
        n++;
    }
    qsort(edges, (size_t)n, sizeof(GenEdge), edge_cmp);
    int m = 0;
    for (int i = 0; i < n; i++) {
        if (m > 0 && edges[m - 1].u == edges[i].u && edges[m - 1].v == edges[i].v) {
            if (edges[i].w < edges[m - 1].w) edges[m - 1].w = edges[i].w;
        } else {
            edges[m++] = edges[i];
        }
    }
    *nout = m;
    return edges;
}

static void write_adjlist(const GenEdge *edges, int n, int V, const char *file) {
    FILE *f = fopen(file, "w");
    if (!f) {
        fprintf(stderr, "gen: cannot write %s\n", file);
        exit(1);
    }
    fprintf(f, "%d %d\n", V, n);
    size_t pos = 0;
    for (int u = 0; u < V; u++) {
        size_t start = pos;
        while (pos < (size_t)n && edges[pos].u == u) pos++;
        fprintf(f, "%d %d", u, (int)(pos - start));
        for (size_t k = start; k < pos; k++) {
            fprintf(f, " %d ", edges[k].v);
#ifdef _WIN32
        fprintf(f, "%I64d", (long long)edges[k].w);
#else
        fprintf(f, "%lld", (long long)edges[k].w);
#endif
        }
        fprintf(f, "\n");
    }
    fprintf(f, "SOURCE 0\n");
    fclose(f);
}

static void write_matrix(const GenEdge *edges, int n, int V, const char *file) {
    int64_t *mat = (int64_t *)malloc((size_t)V * (size_t)V * sizeof(int64_t));
    for (size_t i = 0; i < (size_t)V * (size_t)V; i++) mat[i] = GRAPH_INF;
    for (int i = 0; i < V; i++) mat[(size_t)i * V + i] = 0;
    for (int i = 0; i < n; i++) {
        int64_t *cell = &mat[(size_t)edges[i].u * V + edges[i].v];
        if (edges[i].w < *cell) *cell = edges[i].w;
    }
    FILE *f = fopen(file, "w");
    if (!f) {
        fprintf(stderr, "gen: cannot write %s\n", file);
        exit(1);
    }
    fprintf(f, "%d\n", V);
    for (int i = 0; i < V; i++) {
        for (int j = 0; j < V; j++) {
            if (j) fputc(' ', f);
            fprint_i64(f, mat[(size_t)i * V + j]);
        }
        fputc('\n', f);
    }
    fclose(f);
    free(mat);
}

static void gen_case(const char *kind, int V, int E, uint64_t seed,
                     const char *out) {
    if (V < 1 || E < V - 1) {
        fprintf(stderr, "gen: need V >= 1 and E >= V-1\n");
        exit(1);
    }
    if ((int64_t)V * V < E) {
        fprintf(stderr, "gen: E too large for V=%d (max ", V);
#ifdef _WIN32
        fprintf(stderr, "%I64d)\n", (long long)(int64_t)V * V);
#else
        fprintf(stderr, "%lld)\n", (long long)(int64_t)V * V);
#endif
        exit(1);
    }
    int n = 0;
    GenEdge *edges = build_edges(V, E, seed, &n);
    if (strcmp(kind, "bf") == 0)
        write_adjlist(edges, n, V, out);
    else
        write_matrix(edges, n, V, out);
    free(edges);
    printf("gen: wrote %s: V=%d E=%d\n", out, V, n);
}

static void convert_case(const char *in, const char *out) {
    char err[256];
    AdjList adj;
    int src = -1;
    if (read_weighted_adjlist(in, &adj, &src, err, sizeof(err)) != 0) {
        fprintf(stderr, "gen: %s\n", err);
        exit(1);
    }
    int64_t *mat = (int64_t *)malloc((size_t)adj.V * adj.V * sizeof(int64_t));
    for (size_t i = 0; i < (size_t)adj.V * adj.V; i++) mat[i] = GRAPH_INF;
    for (int i = 0; i < adj.V; i++) mat[(size_t)i * adj.V + i] = 0;
    for (int u = 0; u < adj.V; u++) {
        for (int k = 0; k < adj.deg[u]; k++) {
            int v = adj.nb[u][k];
            int64_t *cell = &mat[(size_t)u * adj.V + v];
            if (adj.wt[u][k] < *cell) *cell = adj.wt[u][k];
        }
    }
    FILE *f = fopen(out, "w");
    if (!f) {
        fprintf(stderr, "gen: cannot write %s\n", out);
        exit(1);
    }
    fprintf(f, "%d\n", adj.V);
    for (int i = 0; i < adj.V; i++) {
        for (int j = 0; j < adj.V; j++) {
            if (j) fputc(' ', f);
            fprint_i64(f, mat[(size_t)i * adj.V + j]);
        }
        fputc('\n', f);
    }
    fclose(f);
    int V = adj.V;
    free(mat);
    free_adjlist(&adj);
    printf("gen: converted %s -> %s (V=%d)\n", in, out, V);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
                "usage:\n"
                "  gen_graph bf V E seed outfile\n"
                "  gen_graph fw V E seed outfile\n"
                "  gen_graph convert bf-file fw-outfile\n");
        return 1;
    }
    if (strcmp(argv[1], "bf") == 0 || strcmp(argv[1], "fw") == 0) {
        if (argc < 6) {
            fprintf(stderr, "usage: gen_graph %s V E seed outfile\n", argv[1]);
            return 1;
        }
        gen_case(argv[1], atoi(argv[2]), atoi(argv[3]),
                 (uint64_t)strtoull(argv[4], NULL, 10), argv[5]);
    } else if (strcmp(argv[1], "convert") == 0) {
        if (argc < 4) {
            fprintf(stderr, "usage: gen_graph convert bf-file fw-outfile\n");
            return 1;
        }
        convert_case(argv[2], argv[3]);
    } else {
        fprintf(stderr, "gen: unknown subcommand '%s'\n", argv[1]);
        return 1;
    }
    return 0;
}
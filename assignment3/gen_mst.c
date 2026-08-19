#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

typedef struct {
    int a, b; /* a < b */
    int64_t w;
} GE;

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

static uint64_t key_of(int u, int v) {
    return ((uint64_t)u << 32) | (uint32_t)v;
}

static int edge_cmp(const void *a, const void *b) {
    const GE *x = (const GE *)a;
    const GE *y = (const GE *)b;
    if (x->w != y->w) return (x->w < y->w) ? -1 : 1;
    if (x->a != y->a) return x->a - y->a;
    return x->b - y->b;
}

typedef struct {
    uint64_t *tab;
    int cap; /* power of two */
} ESet;

static void eset_init(ESet *s, int E) {
    s->cap = 1;
    while (s->cap < 2 * E) s->cap *= 2;
    s->tab = (uint64_t *)calloc((size_t)s->cap, sizeof(uint64_t));
    if (!s->tab) {
        fprintf(stderr, "gen: out of memory\n");
        exit(1);
    }
}

static int eset_contains(const ESet *s, int u, int v) {
    uint64_t key = key_of(u, v);
    int i = (int)(key & (uint64_t)(s->cap - 1));
    while (s->tab[i] && s->tab[i] != key) i = (i + 1) & (s->cap - 1);
    return s->tab[i] == key;
}

static void eset_add(ESet *s, int u, int v) {
    uint64_t key = key_of(u, v);
    int i = (int)(key & (uint64_t)(s->cap - 1));
    while (s->tab[i] && s->tab[i] != key) i = (i + 1) & (s->cap - 1);
    s->tab[i] = key;
}

static int64_t rnd_weight(void) {
    uint64_t r = rnd64() % 100;
    if (r < 5) return 0;
    if (r < 30) return -(int64_t)rnd_int(1, 1000);
    return (int64_t)rnd_int(1, 1000);
}

static GE *build_edges(int V, int E, uint64_t seed, ESet *set) {
    rng_state = seed ? seed : 1;
    GE *edges = (GE *)malloc((size_t)E * sizeof(GE));
    if (!edges) {
        fprintf(stderr, "gen: out of memory\n");
        exit(1);
    }
    int n = 0;

    for (int i = 1; i < V; i++) {
        int p = rnd_int(0, i - 1);
        edges[n].a = p;
        edges[n].b = i;
        edges[n].w = rnd_weight();
        eset_add(set, p, i);
        n++;
    }

    int guard = 0;
    while (n < E && guard < 100 * E + 1000) {
        guard++;
        int u = rnd_int(0, V - 2);
        int v = rnd_int(u + 1, V - 1);
        if (eset_contains(set, u, v)) continue;
        edges[n].a = u;
        edges[n].b = v;
        edges[n].w = rnd_weight();
        eset_add(set, u, v);
        n++;
    }
    if (n < E) {
        fprintf(stderr, "gen: cannot generate %d distinct edges for V=%d "
                        "(too dense)\n", E, V);
        exit(1);
    }
    return edges;
}

static int64_t mst_weight(const GE *edges, int n, int V) {
    GE *copy = (GE *)malloc((size_t)n * sizeof(GE));
    if (!copy) {
        fprintf(stderr, "gen: out of memory\n");
        exit(1);
    }
    memcpy(copy, edges, (size_t)n * sizeof(GE));
    qsort(copy, (size_t)n, sizeof(GE), edge_cmp);

    int *p = (int *)malloc((size_t)V * sizeof(int));
    int *r = (int *)calloc((size_t)V, sizeof(int));
    if (!p || !r) {
        fprintf(stderr, "gen: out of memory\n");
        exit(1);
    }
    for (int i = 0; i < V; i++) p[i] = i;

    int64_t total = 0;
    int m = 0;
    for (int i = 0; i < n && m < V - 1; i++) {
        int a = copy[i].a;
        while (p[a] != a) {
            p[a] = p[p[a]];
            a = p[a];
        }
        int b = copy[i].b;
        while (p[b] != b) {
            p[b] = p[p[b]];
            b = p[b];
        }
        if (a == b) continue;
        if (r[a] < r[b]) {
            int t = a;
            a = b;
            b = t;
        }
        p[b] = a;
        if (r[a] == r[b]) r[a]++;
        total += copy[i].w;
        m++;
    }
    free(copy);
    free(p);
    free(r);
    return total;
}

static void write_adjlist(const GE *edges, int n, int V, const char *file) {
    int *head = (int *)malloc((size_t)V * sizeof(int));
    int *to = (int *)malloc((size_t)2 * n * sizeof(int));
    int64_t *wt = (int64_t *)malloc((size_t)2 * n * sizeof(int64_t));
    int *nxt = (int *)malloc((size_t)2 * n * sizeof(int));
    if (!head || !to || !wt || !nxt) {
        fprintf(stderr, "gen: out of memory\n");
        exit(1);
    }
    for (int i = 0; i < V; i++) head[i] = -1;
    int pos = 0;
    for (int i = 0; i < n; i++) {
        int u = edges[i].a, v = edges[i].b;
        to[pos] = v;
        wt[pos] = edges[i].w;
        nxt[pos] = head[u];
        head[u] = pos++;
        to[pos] = u;
        wt[pos] = edges[i].w;
        nxt[pos] = head[v];
        head[v] = pos++;
    }

    FILE *f = fopen(file, "w");
    if (!f) {
        fprintf(stderr, "gen: cannot write %s\n", file);
        exit(1);
    }
    fprintf(f, "%d %d\n", V, n);
    for (int u = 0; u < V; u++) {
        int cnt = 0;
        for (int e = head[u]; e != -1; e = nxt[e]) cnt++;
        fprintf(f, "%d %d", u, cnt);
        for (int e = head[u]; e != -1; e = nxt[e]) {
            fprintf(f, " %d ", to[e]);
#ifdef _WIN32
            fprintf(f, "%I64d", (long long)wt[e]);
#else
            fprintf(f, "%lld", (long long)wt[e]);
#endif
        }
        fputc('\n', f);
    }
    fclose(f);
    free(head);
    free(to);
    free(wt);
    free(nxt);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr,
                "usage: gen_mst V E seed outfile\n"
                "  generates a random connected weighted undirected graph\n"
                "  with V vertices and E edges in MST adjacency-list format\n");
        return 1;
    }
    int V = atoi(argv[1]);
    int E = atoi(argv[2]);
    uint64_t seed = (uint64_t)strtoull(argv[3], NULL, 10);
    const char *out = argv[4];

    if (V < 1) {
        fprintf(stderr, "gen: V must be >= 1\n");
        return 1;
    }
    if (E < V - 1) {
        fprintf(stderr, "gen: E must be >= V-1 (connected graph)\n");
        return 1;
    }
    int64_t maxE = (int64_t)V * (V - 1) / 2;
    if ((int64_t)E > maxE) {
        fprintf(stderr, "gen: E too large for V=%d (max "
#ifdef _WIN32
                        "%I64d"
#else
                        "%lld"
#endif
                        ")\n", V, (long long)maxE);
        return 1;
    }

    ESet set;
    eset_init(&set, E);
    GE *edges = build_edges(V, E, seed, &set);
    int64_t wt = mst_weight(edges, E, V);
    write_adjlist(edges, E, V, out);
    free(edges);
    free(set.tab);

    printf("gen: wrote %s: V=%d E=%d seed=", out, V, E);
#ifdef _WIN32
    printf("%I64u", (unsigned long long)seed);
#else
    printf("%llu", (unsigned long long)seed);
#endif
    printf(" MST weight=");
#ifdef _WIN32
    printf("%I64d\n", (long long)wt);
#else
    printf("%lld\n", (long long)wt);
#endif
    return 0;
}

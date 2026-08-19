#include "mst_graph.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *skip_ws(const char *p) {
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

static int next_i64(const char **pp, int64_t *out) {
    const char *p = skip_ws(*pp);
    if (!*p) return -1;
    errno = 0;
    char *end = NULL;
    long long v = strtoll(p, &end, 10);
    if (errno != 0 || end == p) return -1;
    *pp = end;
    *out = (int64_t)v;
    return 0;
}

static int parse_int(const char **pp, int *out) {
    int64_t v;
    if (next_i64(pp, &v) != 0 || v > 100000000 || v < -100000000) return -1;
    *out = (int)v;
    return 0;
}

static char *read_whole_file(const char *path, size_t *len, char *err,
                             size_t errsz) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        snprintf(err, errsz, "cannot open input file: %s", path);
        return NULL;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        snprintf(err, errsz, "cannot seek in file: %s", path);
        fclose(f);
        return NULL;
    }
    long sz = ftell(f);
    if (sz < 0) {
        snprintf(err, errsz, "cannot read file size: %s", path);
        fclose(f);
        return NULL;
    }
    rewind(f);
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        snprintf(err, errsz, "out of memory reading file");
        fclose(f);
        return NULL;
    }
    if (sz > 0 && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        snprintf(err, errsz, "short read on file: %s", path);
        free(buf);
        fclose(f);
        return NULL;
    }
    buf[sz] = '\0';
    fclose(f);
    *len = (size_t)sz;
    return buf;
}

static int mst_connected(const AdjList *g) {
    int V = g->V;
    char *seen = (char *)calloc((size_t)V, sizeof(char));
    int *queue = (int *)malloc((size_t)V * sizeof(int));
    if (!seen || !queue) {
        free(seen);
        free(queue);
        return -1;
    }
    int head = 0, tail = 0, count = 0;
    seen[0] = 1;
    queue[tail++] = 0;
    while (head < tail) {
        int u = queue[head++];
        count++;
        for (int k = 0; k < g->deg[u]; k++) {
            int v = g->nb[u][k];
            if (!seen[v]) {
                seen[v] = 1;
                queue[tail++] = v;
            }
        }
    }
    free(seen);
    free(queue);
    return count == V;
}

int read_mst_adjlist(const char *path, AdjList *g, char *err, size_t errsz) {
    memset(g, 0, sizeof(*g));

    size_t len;
    char *buf = read_whole_file(path, &len, err, errsz);
    if (!buf) return -1;

    const char *p = buf;

    int V = 0, E = 0;
    if (parse_int(&p, &V) != 0 || V < 1) {
        snprintf(err, errsz, "file %s: expected vertex count V >= 1", path);
        free(buf);
        return -1;
    }
    if (parse_int(&p, &E) != 0 || E < 0) {
        snprintf(err, errsz, "file %s: expected edge count E >= 0", path);
        free(buf);
        return -1;
    }

    g->V = V;
    g->directed = 0;
    g->weighted = 1;

    g->deg = (int *)calloc((size_t)V, sizeof(int));
    g->nb = (int **)calloc((size_t)V, sizeof(int *));
    g->wt = (int64_t **)calloc((size_t)V, sizeof(int64_t *));
    if (!g->deg || !g->nb || !g->wt) {
        snprintf(err, errsz, "out of memory");
        free_adjlist(g);
        free(buf);
        return -1;
    }

    int sumdeg = 0;
    for (int v = 0; v < V; v++) {
        int u = -1;
        if (parse_int(&p, &u) != 0 || u != v) {
            snprintf(err, errsz,
                     "file %s: vertex order broken, expected %d", path, v);
            free_adjlist(g);
            free(buf);
            return -1;
        }
        int deg = -1;
        if (parse_int(&p, &deg) != 0 || deg < 0) {
            snprintf(err, errsz, "file %s: bad degree for vertex %d", path, v);
            free_adjlist(g);
            free(buf);
            return -1;
        }
        g->deg[v] = deg;
        sumdeg += deg;
        if (deg > 0) {
            g->nb[v] = (int *)malloc((size_t)deg * sizeof(int));
            g->wt[v] = (int64_t *)malloc((size_t)deg * sizeof(int64_t));
            if (!g->nb[v] || !g->wt[v]) {
                snprintf(err, errsz, "out of memory");
                free_adjlist(g);
                free(buf);
                return -1;
            }
        }
        for (int k = 0; k < deg; k++) {
            int nbr = -1;
            if (parse_int(&p, &nbr) != 0) {
                snprintf(err, errsz,
                         "file %s: bad neighbour %d of vertex %d",
                         path, k, v);
                free_adjlist(g);
                free(buf);
                return -1;
            }
            int64_t w = 0;
            if (next_i64(&p, &w) != 0) {
                snprintf(err, errsz,
                         "file %s: bad weight for neighbour %d of vertex %d",
                         path, k, v);
                free_adjlist(g);
                free(buf);
                return -1;
            }
            if (nbr < 0 || nbr >= V) {
                snprintf(err, errsz,
                         "file %s: neighbour %d of vertex %d out of range [0,%d)",
                         path, nbr, v, V);
                free_adjlist(g);
                free(buf);
                return -1;
            }
            if (nbr == v) {
                snprintf(err, errsz,
                         "file %s: self-loop on vertex %d is not allowed", path, v);
                free_adjlist(g);
                free(buf);
                return -1;
            }
            for (int j = 0; j < k; j++) {
                if (g->nb[v][j] == nbr) {
                    snprintf(err, errsz,
                             "file %s: duplicate neighbour %d in vertex %d",
                             path, nbr, v);
                    free_adjlist(g);
                    free(buf);
                    return -1;
                }
            }
            g->nb[v][k] = nbr;
            g->wt[v][k] = w;
        }
    }

    if (sumdeg != 2 * E) {
        snprintf(err, errsz,
                 "file %s: declared E=%d but degrees sum to %d (must be 2E)",
                 path, E, sumdeg);
        free_adjlist(g);
        free(buf);
        return -1;
    }

    for (int u = 0; u < V; u++) {
        for (int k = 0; k < g->deg[u]; k++) {
            int v = g->nb[u][k];
            int found = 0;
            for (int j = 0; j < g->deg[v]; j++) {
                if (g->nb[v][j] == u) {
                    if (g->wt[v][j] != g->wt[u][k]) {
                        snprintf(err, errsz,
                                 "file %s: weight mismatch on edge (%d,%d)",
                                 path, u, v);
                        free_adjlist(g);
                        free(buf);
                        return -1;
                    }
                    found = 1;
                    break;
                }
            }
            if (!found) {
                snprintf(err, errsz,
                         "file %s: edge (%d,%d) not mirrored in vertex %d",
                         path, u, v, v);
                free_adjlist(g);
                free(buf);
                return -1;
            }
        }
    }

    const char *q = skip_ws(p);
    if (*q) {
        snprintf(err, errsz, "file %s: trailing data after vertex list", path);
        free_adjlist(g);
        free(buf);
        return -1;
    }

    if (V > 1) {
        int connected = mst_connected(g);
        if (connected < 0) {
            snprintf(err, errsz, "out of memory");
            free_adjlist(g);
            free(buf);
            return -1;
        }
        if (!connected) {
            snprintf(err, errsz,
                     "file %s: graph is not connected (MST requires connectivity)",
                     path);
            free_adjlist(g);
            free(buf);
            return -1;
        }
    }

    g->E = sumdeg; /* CSR stores each undirected edge twice (both directions) */
    free(buf);
    return 0;
}

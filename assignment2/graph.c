#include "graph.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *p;
} Tok;

static char *next_tok(Tok *t) {
    while (*t->p && isspace((unsigned char)*t->p)) t->p++;
    if (!*t->p) return NULL;
    char *tok = t->p;
    while (*t->p && !isspace((unsigned char)*t->p)) t->p++;
    if (*t->p) *t->p++ = '\0';
    return tok;
}

static int parse_i64(const char *tok, int64_t *out) {
    if (!tok || !*tok) return -1;
    errno = 0;
    char *end = NULL;
    long long v = strtoll(tok, &end, 10);
    if (errno != 0 || end == tok || *end != '\0') return -1;
    if (v > ((int64_t)1 << 62) || v < -((int64_t)1 << 62)) return -1;
    *out = (int64_t)v;
    return 0;
}

static int parse_int(const char *tok, int *out) {
    int64_t v;
    if (parse_i64(tok, &v) != 0 || v > 100000000 || v < -100000000) return -1;
    *out = (int)v;
    return 0;
}

static char *read_whole_file(const char *path, size_t *len, char *err, size_t errsz) {
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

int read_weighted_adjlist(const char *path, AdjList *g, int *src,
                          char *err, size_t errsz) {
    memset(g, 0, sizeof(*g));
    *src = -1;

    size_t len;
    char *buf = read_whole_file(path, &len, err, errsz);
    if (!buf) return -1;

    Tok t = { buf };
    char *tok = next_tok(&t);

    int V = 0, E = 0;
    if (!tok || parse_int(tok, &V) != 0) {
        snprintf(err, errsz, "file %s: expected vertex count V", path);
        free(buf);
        return -1;
    }
    tok = next_tok(&t);
    if (!tok || parse_int(tok, &E) != 0) {
        snprintf(err, errsz, "file %s: expected edge count E", path);
        free(buf);
        return -1;
    }

    g->V = V;
    g->directed = 1;
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
        tok = next_tok(&t);
        if (!tok) {
            snprintf(err, errsz, "file %s: line for vertex %d missing", path, v);
            free_adjlist(g);
            free(buf);
            return -1;
        }
        int u = -1;
        if (parse_int(tok, &u) != 0 || u != v) {
            snprintf(err, errsz,
                     "file %s: vertex order broken, expected %d got '%s'",
                     path, v, tok);
            free_adjlist(g);
            free(buf);
            return -1;
        }
        tok = next_tok(&t);
        int deg = 0;
        if (!tok || parse_int(tok, &deg) != 0 || deg < 0) {
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
            tok = next_tok(&t);
            int nbr = -1;
            if (!tok || parse_int(tok, &nbr) != 0) {
                snprintf(err, errsz,
                         "file %s: bad neighbour %d of vertex %d", path, k, v);
                free_adjlist(g);
                free(buf);
                return -1;
            }
            tok = next_tok(&t);
            int64_t w = 0;
            if (!tok || parse_i64(tok, &w) != 0) {
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
            if (!g->directed && w < 0) {
                snprintf(err, errsz,
                         "file %s: negative weight %lld on undirected edge (%d,%d)",
                         path, (long long)w, v, nbr);
                free_adjlist(g);
                free(buf);
                return -1;
            }
            g->nb[v][k] = nbr;
            g->wt[v][k] = w;
        }
    }

    if (sumdeg != E) {
        snprintf(err, errsz,
                 "file %s: declared E=%d but degrees sum to %d", path, E, sumdeg);
        free_adjlist(g);
        free(buf);
        return -1;
    }

    tok = next_tok(&t);
    if (!tok || strcmp(tok, "SOURCE") != 0) {
        snprintf(err, errsz, "file %s: missing SOURCE line", path);
        free_adjlist(g);
        free(buf);
        return -1;
    }
    tok = next_tok(&t);
    int s = -1;
    if (!tok || parse_int(tok, &s) != 0 || s < 0 || s >= V) {
        snprintf(err, errsz, "file %s: bad SOURCE vertex", path);
        free_adjlist(g);
        free(buf);
        return -1;
    }
    tok = next_tok(&t);
    if (tok) {
        snprintf(err, errsz, "file %s: trailing data after SOURCE line", path);
        free_adjlist(g);
        free(buf);
        return -1;
    }

    g->E = sumdeg;
    *src = s;
    free(buf);
    return 0;
}

int read_dense_matrix(const char *path, int *V, int64_t **mat,
                      char *err, size_t errsz) {
    *V = -1;
    *mat = NULL;

    size_t len;
    char *buf = read_whole_file(path, &len, err, errsz);
    if (!buf) return -1;

    Tok t = { buf };
    char *tok = next_tok(&t);
    int n = 0;
    if (!tok || parse_int(tok, &n) != 0 || n < 1 || n > 10000) {
        snprintf(err, errsz, "file %s: bad matrix dimension V", path);
        free(buf);
        return -1;
    }

    int64_t *m = (int64_t *)malloc((size_t)n * (size_t)n * sizeof(int64_t));
    if (!m) {
        snprintf(err, errsz, "out of memory for %dx%d matrix", n, n);
        free(buf);
        return -1;
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            tok = next_tok(&t);
            if (!tok) {
                snprintf(err, errsz,
                         "file %s: missing entry [%d][%d] (need %d values)",
                         path, i, j, n * n);
                free(m);
                free(buf);
                return -1;
            }
            if (strcmp(tok, "INF") == 0 || strcmp(tok, "inf") == 0) {
                m[(size_t)i * n + j] = GRAPH_INF;
            } else {
                int64_t v = 0;
                if (parse_i64(tok, &v) != 0) {
                    snprintf(err, errsz,
                             "file %s: bad entry '%s' at [%d][%d]",
                             path, tok, i, j);
                    free(m);
                    free(buf);
                    return -1;
                }
                m[(size_t)i * n + j] = v;
            }
        }
    }
    tok = next_tok(&t);
    if (tok) {
        snprintf(err, errsz, "file %s: too many entries in matrix", path);
        free(m);
        free(buf);
        return -1;
    }

    for (int i = 0; i < n; i++) {
        if (m[(size_t)i * n + i] != 0) {
            snprintf(err, errsz,
                     "file %s: matrix diagonal entry [%d][%d] must be 0",
                     path, i, i);
            free(m);
            free(buf);
            return -1;
        }
    }

    *V = n;
    *mat = m;
    free(buf);
    return 0;
}

void AdjListToCSR(const AdjList *g, CSRGraph *csr) {
    int V = g->V;
    int E = g->E;

    csr->V = V;
    csr->E = E;
    csr->row_ptr = (int *)malloc(((size_t)V + 1) * sizeof(int));
    csr->col_idx = (E > 0) ? (int *)malloc((size_t)E * sizeof(int)) : NULL;
    csr->values = (E > 0) ? (int64_t *)malloc((size_t)E * sizeof(int64_t)) : NULL;

    size_t pos = 0;
    for (int u = 0; u < V; u++) {
        csr->row_ptr[u] = (int)pos;
        for (int k = 0; k < g->deg[u]; k++) {
            csr->col_idx[pos] = g->nb[u][k];
            csr->values[pos] = g->wt[u][k];
            pos++;
        }
    }
    csr->row_ptr[V] = (int)pos;
}

void free_adjlist(AdjList *g) {
    if (!g) return;
    if (g->nb) {
        for (int u = 0; u < g->V; u++) free(g->nb[u]);
    }
    if (g->wt) {
        for (int u = 0; u < g->V; u++) free(g->wt[u]);
    }
    free(g->deg);
    free(g->nb);
    free(g->wt);
    memset(g, 0, sizeof(*g));
}

void free_csr(CSRGraph *g) {
    if (!g) return;
    free(g->row_ptr);
    free(g->col_idx);
    free(g->values);
    memset(g, 0, sizeof(*g));
}

void fprint_i64(FILE *f, int64_t v) {
    if (v == GRAPH_INF) {
        fputs("INF", f);
        return;
    }
#ifdef _WIN32
    fprintf(f, "%I64d", (long long)v);
#else
    fprintf(f, "%lld", (long long)v);
#endif
}
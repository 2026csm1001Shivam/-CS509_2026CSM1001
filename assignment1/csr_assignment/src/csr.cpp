#include "csr.h"

#include <algorithm>

bool build_csr(const AdjacencyList &adj, CSRGraph &g) {
    g = CSRGraph();
    g.V = adj.V;
    g.weighted = adj.weighted;

g.row_ptr.assign(static_cast<std::size_t>(adj.V) + 1, 0);
    for (int v = 0; v < adj.V; ++v) {
        g.row_ptr[static_cast<std::size_t>(v) + 1] =
            g.row_ptr[static_cast<std::size_t>(v)] +
            static_cast<int>(adj.neighbors[static_cast<std::size_t>(v)].size());
    }

    const std::size_t total = static_cast<std::size_t>(g.row_ptr[adj.V]);
    g.col_idx.assign(total, 0);
    if (adj.weighted) g.values.assign(total, 0.0);

std::vector<int> fill(g.row_ptr);
    for (int v = 0; v < adj.V; ++v) {
        const std::vector<int> &nb = adj.neighbors[static_cast<std::size_t>(v)];
        std::size_t pos = static_cast<std::size_t>(fill[static_cast<std::size_t>(v)]);
        for (std::size_t e = 0; e < nb.size(); ++e) {
            g.col_idx[pos + e] = nb[e];
            if (adj.weighted) {
                g.values[pos + e] = adj.weights[static_cast<std::size_t>(v)][e];
            }
        }
    }
    return true;
}

bool csr_verify(const AdjacencyList &adj, const CSRGraph &g) {
    if (g.V != adj.V) return false;
    if (g.row_ptr.size() != static_cast<std::size_t>(adj.V) + 1) return false;
    if (g.row_ptr[0] != 0) return false;
    if (g.weighted != adj.weighted) return false;

    std::size_t total = 0;
    for (int v = 0; v < adj.V; ++v) {
        if (g.row_ptr[static_cast<std::size_t>(v) + 1] <
            g.row_ptr[static_cast<std::size_t>(v)]) {
            return false;
        }
        total = static_cast<std::size_t>(g.row_ptr[static_cast<std::size_t>(v) + 1]);
    }
    if (g.col_idx.size() != total) return false;
    if (adj.weighted && g.values.size() != total) return false;

    for (int v = 0; v < adj.V; ++v) {
        const std::size_t begin = static_cast<std::size_t>(g.row_ptr[v]);
        const std::size_t end = static_cast<std::size_t>(g.row_ptr[static_cast<std::size_t>(v) + 1]);
        const std::vector<int> &nb = adj.neighbors[static_cast<std::size_t>(v)];
        if (end - begin != nb.size()) return false;

        for (std::size_t e = 0; e < nb.size(); ++e) {
            const int c = g.col_idx[begin + e];
            if (c < 0 || c >= adj.V) return false;
            if (c != nb[e]) return false;
            if (adj.weighted && g.values[begin + e] != adj.weights[static_cast<std::size_t>(v)][e]) {
                return false;
            }
        }
    }
    return true;
}

double csr_scan_checksum(const CSRGraph &g) {
    double sum = 0.0;
    for (int v = 0; v < g.V; ++v) {
        const std::size_t begin = static_cast<std::size_t>(g.row_ptr[v]);
        const std::size_t end = static_cast<std::size_t>(g.row_ptr[static_cast<std::size_t>(v) + 1]);
        for (std::size_t e = begin; e < end; ++e) {
            sum += g.col_idx[e];
            if (g.weighted) sum += g.values[e];
        }
    }
    return sum;
}

#ifndef CSR_H
#define CSR_H

#include <vector>

struct AdjacencyList {
    int V = 0;                              
    bool weighted = false;                  
    std::vector<std::vector<int> > neighbors;
    std::vector<std::vector<double> > weights;  
};

struct CSRGraph {
    int V = 0;                               
    std::vector<int> row_ptr;                
    std::vector<int> col_idx;                
    std::vector<double> values;              
    bool weighted = false;
};

bool build_csr(const AdjacencyList &adj, CSRGraph &g);

bool csr_verify(const AdjacencyList &adj, const CSRGraph &g);

double csr_scan_checksum(const CSRGraph &g);

#endif

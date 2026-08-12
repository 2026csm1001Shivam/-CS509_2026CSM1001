#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include "csr.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

static const char *DEFAULT_TESTS_DIR = "csr_assignment" PATH_SEP "tests";
static const char *FALLBACK_TESTS_DIR = "tests";
static const char *DEFAULT_OUTS_DIR  = "csr_assignment" PATH_SEP "outputs";
static const char *FALLBACK_OUTS_DIR  = "outputs";

static std::string trim(const std::string &s) {
    std::size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    std::size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static bool file_exists(const std::string &path) {
    std::ifstream f(path.c_str());
    return f.good();
}

static bool dir_exists(const std::string &path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
#endif
}

static void ensure_output_dir(const std::string &dir) {
    if (dir_exists(dir)) return;
#ifdef _WIN32
    _mkdir(dir.c_str());
#else
    mkdir(dir.c_str(), 0777);
#endif
}

static std::string strip_ext(const std::string &name) {
    std::size_t dot = name.find_last_of('.');
    return (dot == std::string::npos) ? name : name.substr(0, dot);
}

static std::string base_name(const std::string &path) {
    std::size_t slash = path.find_last_of("/\\");
    return strip_ext(path.substr(slash == std::string::npos ? 0 : slash + 1));
}

static std::vector<std::string> list_test_files(const std::string &dir) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + PATH_SEP "csr_*.txt").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do { files.push_back(fd.cFileName); } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
#else
    DIR *d = opendir(dir.c_str());
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            std::string name = e->d_name;
            if (name.compare(0, 4, "csr_") == 0 &&
                name.size() > 4 && name.substr(name.size() - 4) == ".txt")
                files.push_back(name);
        }
        closedir(d);
    }
#endif
    std::sort(files.begin(), files.end());
    return files;
}

static double now_ms() {
#ifdef _WIN32
    static LARGE_INTEGER freq = {0, 0};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return static_cast<double>(t.QuadPart) * 1000.0 / static_cast<double>(freq.QuadPart);
#else
    return std::chrono::duration<double, std::milli>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

template <typename F>
static double timed_ms_avg(F f, int &runs) {
    double t0 = now_ms();
    f();
    double t1 = now_ms();
    double single = t1 - t0;

    int r = 1;
    if (single < 5.0) {
        if (single > 0.0) {
            r = static_cast<int>(5.0 / single);
        } else {
            r = 10000;  
        }
        if (r < 1) r = 1;
        if (r > 100000) r = 100000;
    }

    double a0 = now_ms();
    for (int i = 0; i < r; ++i) f();
    double a1 = now_ms();

    runs = r;
    return (a1 - a0) / r;
}

static bool parse_graph_file(const std::string &path, AdjacencyList &adj, int &source) {
    std::ifstream in(path.c_str());
    if (!in) return false;

    adj = AdjacencyList();
    source = -1;
    int V = 0;
    bool header = false;

    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::istringstream ss(line);
        if (!header) {
            if (!(ss >> V) || V <= 0) return false;
            adj.V = V;
            adj.neighbors.assign(static_cast<std::size_t>(V), std::vector<int>());
            adj.weights.assign(static_cast<std::size_t>(V), std::vector<double>());
            header = true;
            continue;
        }

        std::string first;
        ss >> first;
        if (first == "SOURCE") {
            if (!(ss >> source)) return false;
            continue;
        }

        int u = -1, deg = -1;
        {
            std::istringstream ps(first);
            if (!(ps >> u)) return false;
        }
        if (!(ss >> deg)) return false;
        if (u < 0 || u >= V) return false;

        std::vector<int> tok;
        int x;
        while (ss >> x) tok.push_back(x);

        if (tok.size() == static_cast<std::size_t>(deg)) {

            adj.neighbors[static_cast<std::size_t>(u)] = tok;
        } else if (tok.size() == static_cast<std::size_t>(deg) * 2) {

            adj.weighted = true;
            std::vector<int> nb;
            std::vector<double> wt;
            for (int e = 0; e < deg; ++e) {
                int n = tok[static_cast<std::size_t>(e) * 2];
                double w = static_cast<double>(tok[static_cast<std::size_t>(e) * 2 + 1]);
                if (w <= 0.0) return false;  
                nb.push_back(n);
                wt.push_back(w);
            }
            adj.neighbors[static_cast<std::size_t>(u)] = nb;
            adj.weights[static_cast<std::size_t>(u)] = wt;
        } else {
            return false;
        }
    }

    return header;
}

static int run_case(const std::string &test_path, const std::string &out_dir) {
    AdjacencyList adj;
    int source = -1;
    if (!parse_graph_file(test_path, adj, source)) {
        std::cerr << "Error: cannot parse graph test file '" << test_path
                  << "' (expected: 'V E', then 'u degree neighbors...' per vertex, "
                     "with 'SOURCE s' line for weighted/unweighted adjacency lists)\n";
        return EXIT_FAILURE;
    }
    if (source < 0) source = 0;

CSRGraph g;
    double p0 = now_ms();
    build_csr(adj, g);
    double p1 = now_ms();
    double ms_convert = p1 - p0;

    bool ok = csr_verify(adj, g);
    long long stored = g.row_ptr[adj.V];

double checksum = csr_scan_checksum(g);  
    int runs_scan = 1;
    double ms_scan = timed_ms_avg([&]() { csr_scan_checksum(g); }, runs_scan);

    int min_deg = adj.V, max_deg = 0;
    for (int v = 0; v < adj.V; ++v) {
        int d = static_cast<int>(adj.neighbors[static_cast<std::size_t>(v)].size());
        if (d < min_deg) min_deg = d;
        if (d > max_deg) max_deg = d;
    }

    std::string base = base_name(test_path);
    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "Algorithm: CSR\n";
    out << "Test file: " << test_path << "\n";
    out << "Graph: V=" << adj.V << " stored entries=" << stored
        << " weighted=" << (adj.weighted ? "yes" : "no")
        << " source=" << source << "\n";
    out << "Degree: min=" << min_deg << " max=" << max_deg << "\n";
    out << "CSR validation (adj list == CSR): " << (ok ? "PASSED" : "FAILED") << "\n";
    if (adj.V <= 100) {
        out << "row_ptr:";
        for (std::size_t i = 0; i < g.row_ptr.size(); ++i) out << " " << g.row_ptr[i];
        out << "\ncol_idx:";
        for (std::size_t i = 0; i < g.col_idx.size(); ++i) out << " " << g.col_idx[i];
        out << "\n";
        if (adj.weighted) {
            out << "values:";
            for (std::size_t i = 0; i < g.values.size(); ++i) out << " " << g.values[i];
            out << "\n";
        }
    }
    out << "Preprocessing (adj-list -> CSR) time: " << ms_convert << " ms (not counted)\n";
    out << "Execution time (CSR scan): " << ms_scan << " ms\n";
    out << "(average of " << runs_scan << " run(s))\n";
    out << "Scan checksum: " << checksum << "\n";

    std::cout << out.str();

    ensure_output_dir(out_dir);
    std::string out_path = out_dir + PATH_SEP + "output_" + base + ".txt";
    std::ofstream ofs(out_path.c_str());
    if (ofs) { ofs << out.str(); ofs.close(); std::cout << "(output saved to " << out_path << ")\n"; }
    else     { std::cerr << "Warning: could not write " << out_path << "\n"; }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " <input_file|--all>\n"
              << "  <input_file>  path to a single graph test file (csr_*.txt)\n"
              << "  --all         run every csr_*.txt test file in the tests dir\n"
              << "Examples:\n"
              << "  " << prog << " tests/csr_10.txt\n"
              << "  " << prog << " --all\n";
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string tests_dir = dir_exists(DEFAULT_TESTS_DIR) ? DEFAULT_TESTS_DIR : FALLBACK_TESTS_DIR;
    std::string outs_dir  = dir_exists(DEFAULT_OUTS_DIR)  ? DEFAULT_OUTS_DIR  : FALLBACK_OUTS_DIR;

    std::string arg = argv[1];

    if (arg == "--all") {
        std::vector<std::string> files = list_test_files(tests_dir);
        if (files.empty()) {
            std::cerr << "Error: no 'csr_*.txt' test files found in '" << tests_dir << "'\n";
            return EXIT_FAILURE;
        }
        std::cout << "Running all 'csr_*.txt' files found in '" << tests_dir << "':\n\n";
        int overall = EXIT_SUCCESS;
        for (std::size_t i = 0; i < files.size(); ++i) {
            std::cout << "######## " << (i + 1) << "/" << files.size() << " : " << files[i] << " ########\n";
            if (run_case(tests_dir + PATH_SEP + files[i], outs_dir) != EXIT_SUCCESS)
                overall = EXIT_FAILURE;
            std::cout << "\n";
        }
        std::cout << "All test files finished. "
                  << (overall == EXIT_SUCCESS ? "Everything passed." : "Some tests FAILED.") << "\n";
        return overall;
    }

    if (!file_exists(arg)) {
        std::cerr << "Error: input file not found: '" << arg << "'\n";
        return EXIT_FAILURE;
    }
    return run_case(arg, outs_dir);
}

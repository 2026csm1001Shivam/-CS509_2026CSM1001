#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <cctype>
#include "matrix.h"
#include "gemm.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

static const char *DEFAULT_TESTS_DIR = "matmul_assignment" PATH_SEP "assignment_01" PATH_SEP "tests";
static const char *DEFAULT_OUTS_DIR  = "matmul_assignment" PATH_SEP "assignment_01" PATH_SEP "outputs";
static const char *FALLBACK_TESTS_DIR = "assignment_01" PATH_SEP "tests";
static const char *FALLBACK_OUTS_DIR  = "assignment_01" PATH_SEP "outputs";

static const int PRINT_FULL_LIMIT = 1024;  

static std::string to_lower(std::string s) {
    for (std::size_t i = 0; i < s.size(); ++i) s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
    return s;
}

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

static std::vector<std::string> list_test_files(const std::string &dir, const std::string &prefix) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + PATH_SEP + prefix + "*.txt").c_str(), &fd);
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
            if (name.compare(0, prefix.size(), prefix) == 0 &&
                name.size() > 4 && name.substr(name.size() - 4) == ".txt")
                files.push_back(name);
        }
        closedir(d);
    }
#endif
    std::sort(files.begin(), files.end());
    return files;
}

static int parse_int(const std::string &s, int &out) {
    if (s.empty()) return 1;
    std::istringstream ss(s);
    ss >> out;
    return ss.fail() ? 1 : 0;
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

static void render_matrix(const Matrix &M, std::ostringstream &out, bool full) {
    std::ios::fmtflags saved = out.flags();
    std::streamsize prec = out.precision();
    out.unsetf(std::ios::floatfield);
    out.precision(6);
    if (full) {
        for (int i = 0; i < M.rows(); ++i) {
            for (int j = 0; j < M.cols(); ++j) out << M.at(i, j) << " ";
            out << "\n";
        }
    } else {
        out << "(large matrix; first 8 rows and 8 columns shown)\n";
        for (int i = 0; i < 8 && i < M.rows(); ++i) {
            for (int j = 0; j < 8 && j < M.cols(); ++j) out << M.at(i, j) << " ";
            out << "...\n";
        }
        out << "Result checksum: " << matrix_checksum(M) << "\n";
    }
    out.flags(saved);
    out.precision(prec);
}

struct GemmTest {
    int M = 0, K = 0, N = 0, bs = 32;
    std::vector<double> A, B;  
};

static bool parse_gemm_file(const std::string &path, GemmTest &t) {
    std::ifstream in(path.c_str());
    if (!in) return false;

    t = GemmTest();
    std::string line;
    bool header = false;
    std::vector<double> raw;

    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        if (line[0] == '#') {
            std::istringstream ss(line);
            std::string hash, key;
            ss >> hash >> key;
            if (key == "BLOCK" || key == "BLOCK_SIZE" || key == "BS") {
                std::string val;
                ss >> val;
                int v = 0;
                if (parse_int(val, v) == 0 && v > 0) t.bs = v;
            }
            continue;
        }

        if (!header) {
            std::istringstream ss(line);
            if (!(ss >> t.M >> t.K >> t.N) || t.M <= 0 || t.K <= 0 || t.N <= 0) return false;
            header = true;
            continue;
        }

        std::istringstream ss(line);
        double v;
        while (ss >> v) raw.push_back(v);
    }

    const std::size_t need_a = static_cast<std::size_t>(t.M) * t.K;
    const std::size_t need_b = static_cast<std::size_t>(t.K) * t.N;
    if (!header || raw.size() < need_a + need_b) return false;

    t.A.assign(raw.begin(), raw.begin() + static_cast<std::ptrdiff_t>(need_a));
    t.B.assign(raw.begin() + static_cast<std::ptrdiff_t>(need_a),
               raw.begin() + static_cast<std::ptrdiff_t>(need_a + need_b));
    return true;
}

static int run_gemm_case(const std::string &test_path, const std::string &out_dir) {
    GemmTest t;
    if (!parse_gemm_file(test_path, t)) {
        std::cerr << "Error: cannot parse GEMM test file '" << test_path
                  << "' (expected: first line 'M K N', then M rows of A, then K rows of B)\n";
        return EXIT_FAILURE;
    }

    Matrix A(t.M, t.K), B(t.K, t.N);
    for (int i = 0; i < t.M; ++i)
        for (int k = 0; k < t.K; ++k)
            A.at(i, k) = t.A[static_cast<std::size_t>(i) * t.K + k];
    for (int k = 0; k < t.K; ++k)
        for (int j = 0; j < t.N; ++j)
            B.at(k, j) = t.B[static_cast<std::size_t>(k) * t.N + j];

    Matrix C1(t.M, t.N), C2(t.M, t.N);

    int runs_simple = 1, runs_blocked = 1;
    double ms_simple = timed_ms_avg([&]() { matmul_simple(A, B, C1); }, runs_simple);
    double ms_blocked = timed_ms_avg([&]() { matmul_blocked(A, B, C2, t.bs); }, runs_blocked);

    bool ok = matrices_equal(C1, C2, 1e-6);
    double checksum = matrix_checksum(C1);
    bool print_full = static_cast<std::size_t>(t.M) * t.N <= PRINT_FULL_LIMIT;
    std::string base = base_name(test_path);

    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    out << "Algorithm: GEMM Simple\n";
    out << "Test file: " << test_path << "\n";
    out << "Dimensions: A = " << t.M << " x " << t.K << ", B = " << t.K << " x " << t.N
        << ", C = " << t.M << " x " << t.N << "\n";
    out << "Result matrix:\n";
    render_matrix(C1, out, print_full);
    out << "Execution time: " << ms_simple << " ms\n";
    out << "(average of " << runs_simple << " run(s))\n\n";

    out << "Algorithm: GEMM Blocking\n";
    out << "Block size: " << t.bs << "\n";
    out << "Result matrix:\n";
    render_matrix(C2, out, print_full);
    out << "Execution time: " << ms_blocked << " ms\n";
    out << "(average of " << runs_blocked << " run(s))\n\n";

    out << "Correctness (simple == blocked): " << (ok ? "PASSED" : "FAILED") << "\n";
    out << "Result checksum: " << checksum << "\n";
    out << "Speedup (simple/blocked): " << std::setprecision(2)
        << (ms_blocked > 0 ? (ms_simple / ms_blocked) : 0.0) << "x\n";

    std::cout << out.str();

    ensure_output_dir(out_dir);
    std::string out_path = out_dir + PATH_SEP + "output_" + base + ".txt";
    std::ofstream ofs(out_path.c_str());
    if (ofs) { ofs << out.str(); ofs.close(); std::cout << "(output saved to " << out_path << ")\n"; }
    else     { std::cerr << "Warning: could not write " << out_path << "\n"; }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

static void print_usage(const char *prog) {
    std::cerr << "Usage: " << prog << " gemm <input_file|--all>\n"
              << "  <input_file>  path to a single GEMM test file (gemm_test_*.txt)\n"
              << "  --all         run every matching test file in the tests dir\n"
              << "Examples:\n"
              << "  " << prog << " gemm tests/gemm_test_01.txt\n"
              << "  " << prog << " gemm --all\n";
}

static int run_all(const std::string &tests_dir, const std::string &outs_dir) {
    const std::string prefix = "gemm_test_";
    std::vector<std::string> files = list_test_files(tests_dir, prefix);
    if (files.empty()) {
        std::cerr << "Error: no '" << prefix << "*.txt' test files found in '" << tests_dir << "'\n";
        return EXIT_FAILURE;
    }

    std::cout << "Running all '" << prefix << "*.txt' files found in '" << tests_dir << "':\n\n";
    int overall = EXIT_SUCCESS;
    for (std::size_t i = 0; i < files.size(); ++i) {
        std::cout << "######## " << (i + 1) << "/" << files.size() << " : " << files[i] << " ########\n";
        if (run_gemm_case(tests_dir + PATH_SEP + files[i], outs_dir) != EXIT_SUCCESS)
            overall = EXIT_FAILURE;
        std::cout << "\n";
    }
    std::cout << "All test files finished. "
              << (overall == EXIT_SUCCESS ? "Everything passed." : "Some tests FAILED.") << "\n";
    return overall;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string algorithm = to_lower(argv[1]);
    if (algorithm != "gemm") {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    std::string arg = argv[2];
    std::string tests_dir = dir_exists(DEFAULT_TESTS_DIR) ? DEFAULT_TESTS_DIR : FALLBACK_TESTS_DIR;
    std::string outs_dir  = dir_exists(DEFAULT_OUTS_DIR)  ? DEFAULT_OUTS_DIR  : FALLBACK_OUTS_DIR;

    if (arg == "--all") {
        return run_all(tests_dir, outs_dir);
    }

    if (!file_exists(arg)) {
        std::cerr << "Error: input file not found: '" << arg << "'\n";
        return EXIT_FAILURE;
    }

    return run_gemm_case(arg, outs_dir);
}

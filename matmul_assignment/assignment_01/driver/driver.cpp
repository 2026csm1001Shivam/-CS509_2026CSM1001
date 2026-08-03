/*
 * ============================================================
 *  Assignment 01 - Matrix Multiplication: Simple vs Blocked
 *  Dedicated driver program
 *  Course: CS509 - Lab Work, First-Year M.Tech CSE (2026)
 * ============================================================
 *
 *  The driver is separate from the core algorithm library
 *  (assignment_01/src/matmul.cpp). It:
 *    - reads ONE test case from a test file (N and BLOCK size),
 *    - prepares the input matrices (reproducible random fill),
 *    - times ONLY the algorithm calls (simple and blocked),
 *    - verifies that both versions produce the same result,
 *    - prints the computed result and the algorithm execution
 *      time (explicitly in milliseconds, ms).
 *
 *  Timing rule: input reading, parsing, matrix generation,
 *  verification and printing are all OUTSIDE the timed region.
 *  The timer starts immediately before each algorithm call and
 *  stops immediately after it finishes.
 *
 *  Usage:
 *    driver <test_file>    Run a single test case
 *    driver --all          Run every test_XX.txt in the tests dir
 *
 *  Each test file contains exactly ONE test case.
 * ============================================================
 */

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
#include "matmul.h"

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP "\\"
#else
#include <dirent.h>
#include <sys/stat.h>
#define PATH_SEP "/"
#endif

/* Default locations, relative to the repository root
 * (the driver is normally launched from the repo root via
 * `make run-one TEST=...` or `make run-all`). */
static const char *DEFAULT_TESTS_DIR = "assignment_01" PATH_SEP "tests";
static const char *DEFAULT_OUTS_DIR  = "assignment_01" PATH_SEP "outputs";
static const char *FALLBACK_TESTS_DIR = "tests";
static const char *FALLBACK_OUTS_DIR  = "outputs";

/* ------------------- small helpers -------------------------- */

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

/* List all files named test_*.txt in dir, sorted by name. */
static std::vector<std::string> list_test_files(const std::string &dir) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + PATH_SEP "test_*.txt").c_str(), &fd);
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
            if (name.compare(0, 5, "test_") == 0 &&
                name.size() > 4 && name.substr(name.size() - 4) == ".txt")
                files.push_back(name);
        }
        closedir(d);
    }
#endif
    std::sort(files.begin(), files.end());
    return files;
}

/* Parse exactly one test case: lines of the form "KEY value",
 * case-insensitive keys N and BLOCK. '#' starts a comment. */
static bool parse_test_file(const std::string &path, int &n, int &bs) {
    std::ifstream in(path.c_str());
    if (!in) return false;

    n = bs = -1;
    std::string line;
    while (std::getline(in, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ss(line);
        std::string key, value;
        ss >> key >> value;
        if (key.empty() || value.empty()) continue;

        key = to_lower(key);
        if (key == "n")       n  = std::atoi(value.c_str());
        else if (key == "block" || key == "block_size" || key == "bs")
            bs = std::atoi(value.c_str());
    }
    return n > 0 && bs > 0;
}

static std::string strip_ext(const std::string &name) {
    std::size_t dot = name.find_last_of('.');
    return (dot == std::string::npos) ? name : name.substr(0, dot);
}

/* ------------------- run a single case ---------------------- */

static int run_case(const std::string &test_path, const std::string &out_dir) {
    int n = 0, bs = 0;

    /* --- Input reading & parsing (OUTSIDE timed region) --- */
    if (!parse_test_file(test_path, n, bs)) {
        std::cerr << "Error: cannot parse test file '" << test_path
                  << "' (expected keys: N and BLOCK, one test case per file)\n";
        return EXIT_FAILURE;
    }

    /* --- Input preparation (OUTSIDE timed region) ---------- */
    Matrix A(n), B(n), C1(n), C2(n);
    A.fill_random(42);  /* reproducible input matrices          */
    B.fill_random(7);

    /* --- 1) SIMPLE version: timer around the algorithm call -- */
    auto t0 = std::chrono::steady_clock::now();
    matmul_simple(A, B, C1);
    auto t1 = std::chrono::steady_clock::now();
    double ms_simple = std::chrono::duration<double, std::milli>(t1 - t0).count();

    /* --- 2) BLOCKED version: timer around the algorithm call - */
    auto t2 = std::chrono::steady_clock::now();
    matmul_blocked(A, B, C2, bs);
    auto t3 = std::chrono::steady_clock::now();
    double ms_blocked = std::chrono::duration<double, std::milli>(t3 - t2).count();

    /* --- Verification & result extraction (OUTSIDE timed) --- */
    bool ok = matrices_equal(C1, C2, 1e-6);
    double checksum = matrix_checksum(C1);

    std::size_t slash = test_path.find_last_of("/\\");
    std::string base = strip_ext(test_path.substr(slash == std::string::npos ? 0 : slash + 1));

    std::ostringstream out;
    out << std::fixed << std::setprecision(6);
    out << "============================================\n";
    out << " Test case         : " << test_path << "\n";
    out << " Matrix size N     : " << n << " x " << n << "\n";
    out << " Block size (BS)   : " << bs << "\n";
    out << "--------------------------------------------\n";
    out << " Result (checksum of C = A x B): " << checksum << "\n";
    out << " Correctness (simple == blocked): " << (ok ? "PASSED" : "FAILED") << "\n";
    out << "--------------------------------------------\n";
    out << " Algorithm time (simple)  : " << ms_simple  << " ms\n";
    out << " Algorithm time (blocked) : " << ms_blocked << " ms\n";
    out << " Speedup (simple/blocked) : " << std::setprecision(2)
        << (ms_blocked > 0 ? (ms_simple / ms_blocked) : 0.0) << "x\n";
    out << "============================================\n";

    std::cout << out.str();

    /* Optional: print matrices for small N so results can be
     * verified by hand (still outside the timed region). */
    if (n <= 8) {
        std::cout << "Matrix A:\n"; A.print();
        std::cout << "\nMatrix B:\n"; B.print();
        std::cout << "\nResult C (simple):\n"; C1.print();
        std::cout << "\nResult C (blocked):\n"; C2.print();
    }

    /* Save output file for the assignment's outputs/ folder */
    ensure_output_dir(out_dir);
    std::string out_path = out_dir + PATH_SEP + "output_" + base + ".txt";
    std::ofstream ofs(out_path.c_str());
    if (ofs) { ofs << out.str(); ofs.close(); std::cout << "(output saved to " << out_path << ")\n"; }
    else     { std::cerr << "Warning: could not write " << out_path << "\n"; }

    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

/* ---------------------------- main --------------------------- */

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <test_file>\n"
                  << "       " << argv[0] << " --all\n"
                  << "  <test_file> : path to a single test file (e.g. tests/test_01.txt)\n"
                  << "  --all       : run every test_XX.txt in the assignment's tests dir\n";
        return EXIT_FAILURE;
    }

    std::string tests_dir = dir_exists(DEFAULT_TESTS_DIR) ? DEFAULT_TESTS_DIR : FALLBACK_TESTS_DIR;
    std::string outs_dir  = dir_exists(DEFAULT_OUTS_DIR)  ? DEFAULT_OUTS_DIR  : FALLBACK_OUTS_DIR;

    std::string arg = argv[1];

    if (arg == "--all") {
        std::vector<std::string> files = list_test_files(tests_dir);
        if (files.empty()) {
            std::cerr << "Error: no test files found in '" << tests_dir << "'\n";
            return EXIT_FAILURE;
        }
        std::cout << "Running all test files found in '" << tests_dir << "':\n\n";
        int overall = EXIT_SUCCESS;
        for (std::size_t i = 0; i < files.size(); ++i) {
            std::cout << "######## " << (i + 1) << "/" << files.size()
                      << " : " << files[i] << " ########\n";
            if (run_case(tests_dir + PATH_SEP + files[i], outs_dir) != EXIT_SUCCESS)
                overall = EXIT_FAILURE;
            std::cout << "\n";
        }
        std::cout << "All test files finished. "
                  << (overall == EXIT_SUCCESS ? "Everything passed." : "Some tests FAILED.") << "\n";
        return overall;
    }

    if (!file_exists(arg)) {
        std::cerr << "Error: test file not found: '" << arg << "'\n";
        return EXIT_FAILURE;
    }
    return run_case(arg, outs_dir);
}

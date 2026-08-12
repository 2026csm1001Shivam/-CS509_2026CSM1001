#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <cctype>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define EXE_SUFFIX ".exe"
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define EXE_SUFFIX ""
#endif

struct Runner {
    std::string arg;     
    std::string title;
    std::string prefix;  
};

struct Assignment {
    int    id;
    std::string dir;      
    std::string title;
    std::string tests_dir;  
    std::string src_dir;  
    std::string drv_dir;  
    std::vector<Runner> runners;
};

static const Assignment ASSIGNMENTS[] = {
    { 1, "matmul_assignment", "GEMM - Simple vs Blocked",
          "matmul_assignment/assignment_01/tests",
          "matmul_assignment/assignment_01/src",
          "matmul_assignment/assignment_01/driver",
          { { "gemm", "GEMM - Simple vs Blocked (matrix multiplication)", "gemm_test_" } } },
    { 2, "csr_assignment", "CSR Graph - adjacency list to CSR",
          "csr_assignment/tests",
          "csr_assignment/src",
          "csr_assignment/driver",
          { { "", "CSR - adjacency list to CSR conversion + scan", "csr_" } } }
};
static const int NUM_ASSIGNMENTS = 2;

static std::string native(const std::string &p) {
#ifdef _WIN32
    std::string s = p;
    for (std::size_t i = 0; i < s.size(); ++i)
        if (s[i] == '/') s[i] = '\\';
    return s;
#else
    return p;
#endif
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

static std::string driver_bin(const Assignment &a) {
    return a.drv_dir + "/driver" EXE_SUFFIX;
}

static std::vector<std::string> list_test_files(const std::string &dir, const std::string &prefix) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "\\" + prefix + "*.txt").c_str(), &fd);
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

static std::vector<std::string> list_cpp_files(const std::string &dir) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "\\*.cpp").c_str(), &fd);
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
            if (name.size() > 4 && name.substr(name.size() - 4) == ".cpp")
                files.push_back(name);
        }
        closedir(d);
    }
#endif
    std::sort(files.begin(), files.end());
    return files;
}

static std::string find_repo_root() {
    if (dir_exists("matmul_assignment")) return ".";
    if (dir_exists("csr_assignment")) return ".";
    if (dir_exists("../matmul_assignment")) return "..";
    if (dir_exists("../csr_assignment")) return "..";
    return "";
}

static void print_repo_root_error() {
    std::cerr << "Error: could not locate the repository root.\n"
              << "Run the wrapper from inside the repository (e.g. "
                 "./common_wrapper/wrapper), or check that the\n"
              << "directories 'matmul_assignment' and 'csr_assignment' exist\n"
              << "at the repository root.\n";
}

static bool compile_assignment(const Assignment &a) {
    std::vector<std::string> cpps = list_cpp_files(a.src_dir);
    std::string drv = a.drv_dir + "/driver.cpp";
    std::string bin = driver_bin(a);

    if (cpps.empty() || !file_exists(drv)) {
        std::cerr << "Error: cannot compile Assignment " << a.id
                  << ": no .cpp files in '" << a.src_dir << "' or driver missing "
                     "at '" << drv << "'\n";
        return false;
    }

    std::string cmd = "g++ -O2 -Wall -Wextra -std=c++11 -I " + native(a.src_dir);
    for (std::size_t i = 0; i < cpps.size(); ++i)
        cmd += " " + native(a.src_dir + "/" + cpps[i]);
    cmd += " " + native(drv) + " -o " + native(bin);

    std::cout << "Compiling Assignment " << a.id << " (" << a.title << ") ...\n  " << cmd << "\n";
    if (system(cmd.c_str()) != 0) {
        std::cerr << "Error: compilation of Assignment " << a.id << " failed.\n";
        return false;
    }
    std::cout << "Compiled successfully -> " << bin << "\n";
    return true;
}

static bool ensure_driver(const Assignment &a) {
    if (file_exists(driver_bin(a))) return true;
    std::cout << "(driver executable not found; compiling it first)\n";
    return compile_assignment(a);
}

static std::string runner_arg(const Runner &r) {
    return r.arg.empty() ? "" : " " + r.arg;
}

static void run_one_test(const Assignment &a, const Runner &r) {
    std::string tests_dir = a.tests_dir;
    if (!dir_exists(tests_dir)) {
        std::cerr << "Error: tests directory not found: '" << tests_dir << "'\n";
        return;
    }
    std::vector<std::string> files = list_test_files(tests_dir, r.prefix);
    if (files.empty()) {
        std::cerr << "Error: no '" << r.prefix << "*.txt' test files found in '" << tests_dir << "'\n";
        return;
    }

    std::cout << "Test files available for " << r.title << ":\n";
    for (std::size_t i = 0; i < files.size(); ++i)
        std::cout << "  [" << (i + 1) << "] " << files[i] << "\n";
    std::cout << "Enter the test file name (e.g. " << files[0] << "): ";
    std::cout.flush();

    std::string choice;
    if (!(std::cin >> choice)) return;
    choice = trim(choice);

    bool found = false;
    for (std::size_t i = 0; i < files.size(); ++i) {
        if (files[i] == choice) { found = true; break; }
    }
    if (!found) {
        std::cerr << "Error: no such test file: '" << choice
                  << "'. Choose one of the listed names.\n";
        return;
    }
    if (!ensure_driver(a)) return;

    std::string cmd = native(driver_bin(a)) + runner_arg(r) + " " + native(tests_dir + "/" + choice);
    std::cout << "Running " << cmd << "\n";
    if (system(cmd.c_str()) != 0)
        std::cerr << "Error: driver exited with a non-zero status for '" << choice << "'\n";
}

static void run_all_tests(const Assignment &a, const Runner &r) {
    std::string tests_dir = a.tests_dir;
    if (!dir_exists(tests_dir)) {
        std::cerr << "Error: tests directory not found: '" << tests_dir << "'\n";
        return;
    }
    if (list_test_files(tests_dir, r.prefix).empty()) {
        std::cerr << "Error: no '" << r.prefix << "*.txt' test files found in '" << tests_dir << "'\n";
        return;
    }
    if (!ensure_driver(a)) return;

    std::string cmd = native(driver_bin(a)) + runner_arg(r) + " --all";
    std::cout << "Running all test files: " << cmd << "\n";
    if (system(cmd.c_str()) != 0)
        std::cerr << "Error: '--all' run of Assignment " << a.id << " failed.\n";
}

static void print_menu() {
    std::cout << "\n================================================\n"
              << "  CS509 Common Wrapper - Assignment Menu (C++)\n"
              << "================================================\n"
              << "  1. List available assignments\n"
              << "  2. Compile an assignment\n"
              << "  3. Run one test file of an assignment\n"
              << "  4. Run all test files of an assignment\n"
              << "  5. Compile and run all assignments (all algorithms)\n"
              << "  6. Exit\n"
              << "------------------------------------------------\n"
              << "  Enter choice [1-6]: ";
}

static bool select_assignment(int &out_id) {
    std::cout << "Available assignments:\n";
    for (int i = 0; i < NUM_ASSIGNMENTS; ++i)
        std::cout << "  [" << ASSIGNMENTS[i].id << "] " << ASSIGNMENTS[i].title << "\n";
    std::cout << "Enter assignment number: ";
    std::cout.flush();

    std::string s;
    if (!(std::cin >> s)) return false;
    int id = std::atoi(s.c_str());
    for (int i = 0; i < NUM_ASSIGNMENTS; ++i) {
        if (ASSIGNMENTS[i].id == id) { out_id = id; return true; }
    }
    std::cerr << "Error: unknown assignment number '" << s << "'\n";
    return false;
}

static const Assignment *find_assignment(int id) {
    for (int i = 0; i < NUM_ASSIGNMENTS; ++i)
        if (ASSIGNMENTS[i].id == id) return &ASSIGNMENTS[i];
    return NULL;
}

static bool select_runner(const Assignment &a, int &out_idx) {
    std::cout << "Available algorithms for Assignment " << a.id << ":\n";
    for (std::size_t i = 0; i < a.runners.size(); ++i)
        std::cout << "  [" << (i + 1) << "] " << a.runners[i].title << "\n";
    std::cout << "Enter algorithm number: ";
    std::cout.flush();

    std::string s;
    if (!(std::cin >> s)) return false;
    int n = std::atoi(s.c_str());
    if (n >= 1 && static_cast<std::size_t>(n) <= a.runners.size()) {
        out_idx = n - 1;
        return true;
    }
    std::cerr << "Error: unknown algorithm number '" << s << "'\n";
    return false;
}

int main() {
    std::string root = find_repo_root();
    if (root.empty()) { print_repo_root_error(); return EXIT_FAILURE; }
    if (root != ".") {
#ifdef _WIN32
        _chdir(root.c_str());
#else
        chdir(root.c_str());
#endif
    }

    int choice = 0;
    while (true) {
        print_menu();
        if (!(std::cin >> choice)) break;

        if (choice == 1) {
            std::cout << "Available assignments:\n";
            for (int i = 0; i < NUM_ASSIGNMENTS; ++i)
                std::cout << "  [" << ASSIGNMENTS[i].id << "] " << ASSIGNMENTS[i].title
                          << (dir_exists(ASSIGNMENTS[i].dir) ? " (directory present)" : " (DIRECTORY MISSING!)") << "\n";
        }
        else if (choice == 2) {
            int id; if (!select_assignment(id)) continue;
            const Assignment *a = find_assignment(id);
            if (a) compile_assignment(*a);
        }
        else if (choice == 3) {
            int id; if (!select_assignment(id)) continue;
            int ri; if (!select_runner(*find_assignment(id), ri)) continue;
            const Assignment *a = find_assignment(id);
            if (a) run_one_test(*a, a->runners[ri]);
        }
        else if (choice == 4) {
            int id; if (!select_assignment(id)) continue;
            int ri; if (!select_runner(*find_assignment(id), ri)) continue;
            const Assignment *a = find_assignment(id);
            if (a) run_all_tests(*a, a->runners[ri]);
        }
        else if (choice == 5) {
            bool all_ok = true;
            for (int i = 0; i < NUM_ASSIGNMENTS; ++i)
                all_ok = compile_assignment(ASSIGNMENTS[i]) && all_ok;
            if (all_ok) {
                for (int i = 0; i < NUM_ASSIGNMENTS; ++i)
                    for (std::size_t j = 0; j < ASSIGNMENTS[i].runners.size(); ++j)
                        run_all_tests(ASSIGNMENTS[i], ASSIGNMENTS[i].runners[j]);
            } else {
                std::cerr << "Error: not running tests because compilation failed.\n";
            }
        }
        else if (choice == 6) {
            std::cout << "Goodbye.\n";
            break;
        }
        else {
            std::cerr << "Error: invalid choice '" << choice << "'. Enter a number between 1 and 6.\n";
        }
    }
    return EXIT_SUCCESS;
}

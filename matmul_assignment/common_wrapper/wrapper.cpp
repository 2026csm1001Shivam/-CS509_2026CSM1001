/*
 * ============================================================
 *  CS509 Common Wrapper - Assignment Menu (C++)
 *  Course: CS509 - Lab Work, First-Year M.Tech CSE (2026)
 * ============================================================
 *
 *  Repository-level interface for the submitted assignments.
 *  Provides one menu to:
 *    - list the available assignments,
 *    - compile one assignment (driver + core algorithm),
 *    - run one selected test file of an assignment,
 *    - run all test files of an assignment,
 *    - compile and run all submitted algorithms,
 *    - show clear error messages when a requested source file,
 *      test file, or executable is unavailable.
 *
 *  The wrapper only INVOKES the dedicated driver of the
 *  selected assignment; it does not replace it.
 *
 *  Requirements: a C++ compiler (g++/clang++) and make.
 *  Build:        g++ -O2 -Wall -Wextra -std=c++11 -o common_wrapper/wrapper common_wrapper/wrapper.cpp
 *  Run:          ./common_wrapper/wrapper   (any directory; the
 *                wrapper locates the repository automatically)
 * ============================================================
 */

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

/* ------------------------- assignments ---------------------- */
/* Each submitted assignment is registered here. */
struct Assignment {
    int    id;
    std::string dir;      /* assignment_XX            */
    std::string title;
    std::string src_dir;  /* dir/src                  */
    std::string drv_dir;  /* dir/driver               */
};

static const Assignment ASSIGNMENTS[] = {
    { 1, "assignment_01", "Matrix Multiplication - Simple vs Blocked/Tiled",
          "assignment_01/src", "assignment_01/driver" }
};
static const int NUM_ASSIGNMENTS = 1;

/* ------------------------- small helpers -------------------- */

/* Convert a path to native separators so that command lines
 * passed to system() work under both cmd.exe and POSIX sh. */
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

static std::vector<std::string> list_test_files(const std::string &dir) {
    std::vector<std::string> files;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((dir + "\\test_*.txt").c_str(), &fd);
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

/* Locate the repository root from the current directory. */
static std::string find_repo_root() {
    if (dir_exists("assignment_01")) return ".";
    if (dir_exists("../assignment_01")) return "..";
    return "";
}

static void print_repo_root_error() {
    std::cerr << "Error: could not locate the repository root.\n"
              << "Run the wrapper from inside the repository (e.g. "
                 "./common_wrapper/wrapper), or check that the\n"
              << "directory 'assignment_01' exists at the repository root.\n";
}

/* -------------------- compile / run actions ----------------- */

static bool compile_assignment(const Assignment &a) {
    std::string src  = a.src_dir + "/matmul.cpp";
    std::string hdr  = a.src_dir + "/matmul.h";
    std::string drv  = a.drv_dir + "/driver.cpp";
    std::string bin  = driver_bin(a);

    if (!file_exists(src) || !file_exists(hdr) || !file_exists(drv)) {
        std::cerr << "Error: cannot compile Assignment " << a.id
                  << ": one or more source files are missing.\n"
                  << "  Expected: " << src << ", " << hdr << ", " << drv << "\n";
        return false;
    }

    std::string cmd = "g++ -O2 -Wall -Wextra -std=c++11 -I " + native(a.src_dir) + " " +
                      native(src) + " " + native(drv) + " -o " + native(bin);
    std::cout << "Compiling Assignment " << a.id << " ...\n  " << cmd << "\n";
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

static void run_one_test(const Assignment &a) {
    std::string tests_dir = a.dir + "/tests";
    if (!dir_exists(tests_dir)) {
        std::cerr << "Error: tests directory not found: '" << tests_dir << "'\n";
        return;
    }
    std::vector<std::string> files = list_test_files(tests_dir);
    if (files.empty()) {
        std::cerr << "Error: no test files (test_*.txt) found in '" << tests_dir << "'\n";
        return;
    }

    std::cout << "Test files available for Assignment " << a.id << ":\n";
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

    std::string cmd = native(driver_bin(a)) + " " + native(tests_dir + "/" + choice);
    std::cout << "Running " << cmd << "\n";
    if (system(cmd.c_str()) != 0)
        std::cerr << "Error: driver exited with a non-zero status for '" << choice << "'\n";
}

static void run_all_tests(const Assignment &a) {
    std::string tests_dir = a.dir + "/tests";
    if (!dir_exists(tests_dir)) {
        std::cerr << "Error: tests directory not found: '" << tests_dir << "'\n";
        return;
    }
    if (list_test_files(tests_dir).empty()) {
        std::cerr << "Error: no test files (test_*.txt) found in '" << tests_dir << "'\n";
        return;
    }
    if (!ensure_driver(a)) return;

    std::string cmd = native(driver_bin(a)) + " --all";
    std::cout << "Running all test files: " << cmd << "\n";
    if (system(cmd.c_str()) != 0)
        std::cerr << "Error: '--all' run of Assignment " << a.id << " failed.\n";
}

/* ---------------------------- main --------------------------- */

static void print_menu() {
    std::cout << "\n================================================\n"
              << "  CS509 Common Wrapper - Assignment Menu (C++)\n"
              << "================================================\n"
              << "  1. List available assignments\n"
              << "  2. Compile an assignment\n"
              << "  3. Run one test file of an assignment\n"
              << "  4. Run all test files of an assignment\n"
              << "  5. Compile and run all assignments\n"
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
            const Assignment *a = find_assignment(id);
            if (a) run_one_test(*a);
        }
        else if (choice == 4) {
            int id; if (!select_assignment(id)) continue;
            const Assignment *a = find_assignment(id);
            if (a) run_all_tests(*a);
        }
        else if (choice == 5) {
            bool all_ok = true;
            for (int i = 0; i < NUM_ASSIGNMENTS; ++i)
                all_ok = compile_assignment(ASSIGNMENTS[i]) && all_ok;
            if (all_ok) {
                for (int i = 0; i < NUM_ASSIGNMENTS; ++i) run_all_tests(ASSIGNMENTS[i]);
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

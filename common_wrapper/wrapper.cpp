#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <windows.h>
#define ACCESS(p, m) _access((p), (m))
#define FIND_T _finddata_t
#define FIND_FIRST(p, d) _findfirst((p), (d))
#define FIND_NEXT(h, d) _findnext((h), (d))
#define FIND_CLOSE(h) _findclose((h))
#define ATTR_SUBDIR _A_SUBDIR
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define ACCESS(p, m) ::access((p), (m))
#endif

static std::string wrapperDir;

struct Algo {
    std::string label;
    std::string arg;
    std::string exe;
    std::string testsDir;
    std::string prefix;
    std::string cwd;
    bool inRunAll;
};

struct Assignment {
    std::string name;
    std::vector<std::string> buildDirs;
    std::vector<Algo> algos;
};

static bool pathExists(const std::string& p) {
    return ACCESS(p.c_str(), 0) == 0;
}

static std::string nativePath(const std::string& p) {
    std::string r = p;
    for (size_t i = 0; i < r.size(); ++i)
        if (r[i] == '/') r[i] = '\\';
    return r;
}

static std::string quoteIfSpaces(const std::string& p) {
    return p.find(' ') == std::string::npos ? p : "\"" + p + "\"";
}

static std::string joinRepo(const std::string& rel) {
    return wrapperDir + "/" + rel;
}

static std::string withExeSuffix(const std::string& p) {
#ifdef _WIN32
    return pathExists(p) ? p : p + ".exe";
#else
    return p;
#endif
}

static std::vector<std::string> listTestFiles(const std::string& dir,
                                              const std::string& prefix) {
    std::vector<std::string> files;

#ifdef _WIN32
    std::string pattern = nativePath(dir + "/" + prefix + "*.txt");
    FIND_T fd;
    intptr_t h = FIND_FIRST(pattern.c_str(), &fd);
    if (h != -1) {
        do {
            if (!(fd.attrib & ATTR_SUBDIR)) files.push_back(fd.name);
        } while (FIND_NEXT(h, &fd) == 0);
        FIND_CLOSE(h);
    }
#else
    DIR* d = opendir(dir.c_str());
    if (d) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            std::string name = e->d_name;
            if (name.rfind(prefix, 0) == 0 && name.size() > 4 &&
                name.substr(name.size() - 4) == ".txt") {
                std::string full = dir + "/" + name;
                struct stat st;
                if (stat(full.c_str(), &st) == 0 && S_ISREG(st.st_mode))
                    files.push_back(name);
            }
        }
        closedir(d);
    }
#endif

    std::sort(files.begin(), files.end());
    files.erase(std::remove_if(files.begin(), files.end(),
                               [](const std::string& f) {
                                   return f.find("_bad_") != std::string::npos;
                               }),
                files.end());
    return files;
}

static std::vector<Assignment> discoverAssignments() {
    std::vector<Assignment> result;

    Assignment a1;
    a1.name = "Assignment1 (CSR conversion / GEMM)";
    a1.buildDirs.push_back("assignment1/csr_assignment");
    a1.buildDirs.push_back("assignment1/matmul_assignment");
    Algo csr;
    csr.label = "csr";
    csr.arg = "";
    csr.exe = joinRepo("assignment1/csr_assignment/driver/driver");
    csr.testsDir = joinRepo("assignment1/csr_assignment/tests");
    csr.prefix = "csr_";
    csr.cwd = joinRepo("assignment1/csr_assignment");
    csr.inRunAll = true;
    a1.algos.push_back(csr);
    Algo gemm;
    gemm.label = "gemm";
    gemm.arg = "gemm";
    gemm.exe = joinRepo("assignment1/matmul_assignment/assignment_01/driver/driver");
    gemm.testsDir = joinRepo("assignment1/matmul_assignment/assignment_01/tests");
    gemm.prefix = "gemm_test_";
    gemm.cwd = joinRepo("assignment1/matmul_assignment/assignment_01");
    gemm.inRunAll = true;
    a1.algos.push_back(gemm);
    result.push_back(a1);

    Assignment a2;
    a2.name = "Assignment2 (Bellman-Ford / Floyd-Warshall)";
    a2.buildDirs.push_back("assignment2");
    Algo bf;
    bf.label = "bf";
    bf.arg = "bf";
    bf.exe = joinRepo("assignment2/graphs");
    bf.testsDir = joinRepo("assignment2");
    bf.prefix = "bf_";
    bf.cwd = joinRepo("assignment2");
    bf.inRunAll = true;
    a2.algos.push_back(bf);
    Algo fw;
    fw.label = "fw";
    fw.arg = "fw";
    fw.exe = joinRepo("assignment2/graphs");
    fw.testsDir = joinRepo("assignment2");
    fw.prefix = "fw_";
    fw.cwd = joinRepo("assignment2");
    fw.inRunAll = true;
    a2.algos.push_back(fw);
    result.push_back(a2);

    Assignment a3;
    a3.name = "Assignment3 (MST: Kruskal / Prim)";
    a3.buildDirs.push_back("assignment3");
    const char* a3algos[] = { "kruskal", "prim", "both" };
    for (int i = 0; i < 3; ++i) {
        Algo a;
        a.label = a3algos[i];
        a.arg = a3algos[i];
        a.exe = joinRepo("assignment3/mst");
        a.testsDir = joinRepo("assignment3");
        a.prefix = "mst_";
        a.cwd = joinRepo("assignment3");
        a.inRunAll = (a.label == "both");
        a3.algos.push_back(a);
    }
    result.push_back(a3);

    return result;
}

static void listAssignments(const std::vector<Assignment>& assignments) {
    std::cout << "Detected assignments:\n";
    for (size_t i = 0; i < assignments.size(); ++i) {
        std::cout << "  " << (i + 1) << ") " << assignments[i].name << "\n";
        for (size_t j = 0; j < assignments[i].algos.size(); ++j) {
            const Algo& a = assignments[i].algos[j];
            std::cout << "       - " << a.label << " (tests: " << a.prefix
                      << "*.txt, exe: " << nativePath(a.exe) << ")\n";
        }
    }
}

static bool compileAssignment(const Assignment& a) {
    std::string make = "make";
#ifdef _WIN32
    if (std::system("where make >nul 2>&1") != 0) make = "mingw32-make";
#endif
    bool ok = true;
    for (size_t i = 0; i < a.buildDirs.size(); ++i) {
        std::string dir = joinRepo(a.buildDirs[i]);
        if (!pathExists(dir + "/Makefile")) {
            std::cerr << "Error: no Makefile found in " << dir << "\n";
            ok = false;
            continue;
        }
        std::string cmd = make + " -C " + quoteIfSpaces(nativePath(dir));
        std::cout << "Compiling " << a.name << " ... " << cmd << "\n";
        if (std::system(cmd.c_str()) != 0) {
            std::cerr << "Error: compilation failed in " << dir << "\n";
            ok = false;
        }
    }
    for (size_t i = 0; i < a.algos.size(); ++i) {
        const std::string exe = withExeSuffix(a.algos[i].exe);
        if (!pathExists(exe)) {
            std::cerr << "Error: executable missing at " << nativePath(exe)
                      << "\n";
            ok = false;
        }
    }
    if (ok) std::cout << a.name << " compiled successfully.\n";
    return ok;
}

static std::string runCmd(const Algo& a, const std::string& file) {
    std::string cmd = quoteIfSpaces(nativePath(withExeSuffix(a.exe)));
    if (!a.arg.empty()) cmd += " " + a.arg;
    cmd += " " + quoteIfSpaces(nativePath(file));
    return cmd;
}

static bool runOneTest(const Assignment& ass, const Algo& a,
                       std::string testFile) {
    std::string exe = withExeSuffix(a.exe);
    if (!pathExists(exe)) {
        std::cerr << "Error: executable not built for " << ass.name
                  << " (" << nativePath(exe)
                  << "). Compile it first (menu option 2).\n";
        return false;
    }
    if (!pathExists(testFile))
        testFile = a.testsDir + "/" + testFile;
    if (!pathExists(testFile)) {
        std::cerr << "Error: test file not found: " << testFile << "\n";
        return false;
    }
    std::string cwd = nativePath(a.cwd);
#ifdef _WIN32
    _chdir(cwd.c_str());
#else
    chdir(cwd.c_str());
#endif
    std::string cmd = runCmd(a, testFile);
    std::cout << "Running: " << cmd << "\n";
    return std::system(cmd.c_str()) == 0;
}

static bool runAllTestsFor(const Assignment& a, const Algo& algo) {
    if (!pathExists(algo.testsDir)) {
        std::cerr << "Error: tests directory not found: " << algo.testsDir
                  << "\n";
        return false;
    }
    std::vector<std::string> files = listTestFiles(algo.testsDir, algo.prefix);
    if (files.empty()) {
        std::cerr << "Error: no '" << algo.prefix
                  << "*.txt' test files found in '" << algo.testsDir << "'\n";
        return false;
    }
    std::cout << "\n--- " << a.name << " [" << algo.label << "] ---\n";
    bool ok = true;
    for (size_t i = 0; i < files.size(); ++i) {
        if (!runOneTest(a, algo, algo.testsDir + "/" + files[i])) ok = false;
    }
    return ok;
}

static bool runAllTests(const Assignment& a, const std::string& algoName) {
    bool ok = true;
    for (size_t i = 0; i < a.algos.size(); ++i) {
        if (algoName == "auto" ? a.algos[i].inRunAll
                               : a.algos[i].label == algoName) {
            if (!runAllTestsFor(a, a.algos[i])) ok = false;
        }
    }
    return ok;
}

static std::string promptAlgorithm(const Assignment& a) {
    std::cout << "Algorithm (";
    for (size_t i = 0; i < a.algos.size(); ++i)
        std::cout << a.algos[i].label << "/";
    std::cout << "auto; default auto): ";
    std::string algo;
    std::cin >> algo;
    if (algo.empty()) algo = "auto";
    bool valid = (algo == "auto");
    for (size_t i = 0; i < a.algos.size() && !valid; ++i)
        if (a.algos[i].label == algo) valid = true;
    if (!valid) {
        std::cerr << "Error: invalid algorithm; using auto.\n";
        algo = "auto";
    }
    return algo;
}

static int promptAssignmentChoice(const std::vector<Assignment>& assignments) {
    listAssignments(assignments);
    if (assignments.empty()) return -1;

    std::cout << "Select assignment number: ";
    int choice;
    if (!(std::cin >> choice) || choice < 1 ||
        choice > static_cast<int>(assignments.size())) {
        std::cin.clear();
        std::cin.ignore(1 << 20, '\n');
        std::cerr << "Error: invalid selection.\n";
        return -1;
    }
    return choice - 1;
}

static std::string resolveWrapperDir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(NULL, buf, MAX_PATH);
    std::string path(buf, n);
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) path = path.substr(0, pos);
    size_t pos2 = path.find_last_of("/\\");
    if (pos2 != std::string::npos) path = path.substr(0, pos2);
    return path;
#else
    return "..";
#endif
}

int main() {
    wrapperDir = resolveWrapperDir();
    while (true) {
        std::vector<Assignment> assignments = discoverAssignments();

        std::cout << "\n=== CS509 Common Wrapper (Assignments 1-3) ===\n"
                  << "1) List assignments\n"
                  << "2) Compile an assignment\n"
                  << "3) Run one test file for an assignment\n"
                  << "4) Run all test files for an assignment\n"
                  << "5) Compile and run ALL assignments\n"
                  << "6) Exit\n"
                  << "Choice: ";

        int choice;
        if (!(std::cin >> choice)) break;

        switch (choice) {
            case 1:
                listAssignments(assignments);
                break;
            case 2: {
                int idx = promptAssignmentChoice(assignments);
                if (idx >= 0) compileAssignment(assignments[idx]);
                break;
            }
            case 3: {
                int idx = promptAssignmentChoice(assignments);
                if (idx >= 0) {
                    std::string algoName = promptAlgorithm(assignments[idx]);
                    const Algo* algo = NULL;
                    for (size_t i = 0; i < assignments[idx].algos.size(); ++i) {
                        if (algoName == "auto") {
                            if (assignments[idx].algos[i].inRunAll) {
                                algo = &assignments[idx].algos[i];
                                break;
                            }
                        } else if (assignments[idx].algos[i].label == algoName) {
                            algo = &assignments[idx].algos[i];
                            break;
                        }
                    }
                    if (!algo) {
                        std::cerr << "Error: no algorithm selected.\n";
                        break;
                    }
                    std::cout << "Enter test file name (e.g., "
                              << algo->prefix << "10.txt): ";
                    std::string tf;
                    std::cin >> tf;
                    runOneTest(assignments[idx], *algo, tf);
                }
                break;
            }
            case 4: {
                int idx = promptAssignmentChoice(assignments);
                if (idx >= 0) {
                    std::string algoName = promptAlgorithm(assignments[idx]);
                    runAllTests(assignments[idx], algoName);
                }
                break;
            }
            case 5: {
                if (assignments.empty()) {
                    std::cout << "No assignments to build.\n";
                    break;
                }
                for (size_t i = 0; i < assignments.size(); ++i) {
                    std::cout << "\n=== " << assignments[i].name << " ===\n";
                    if (compileAssignment(assignments[i]))
                        runAllTests(assignments[i], "auto");
                }
                break;
            }
            case 6:
                std::cout << "Goodbye.\n";
                return 0;
            default:
                std::cerr << "Error: invalid choice.\n";
        }
    }

    return 0;
}
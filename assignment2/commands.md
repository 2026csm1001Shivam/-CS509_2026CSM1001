cd C:\iit_ropar_doc\CS509_2026CSM1001\assignment2
mingw32-make                 # build (or: make) -> graphs.exe, gen_graph.exe
.\graphs bf bf_10.txt        # Bellman-Ford
.\graphs fw fw_10.txt        # Floyd-Warshall
.\graphs                     # interactive menu
.\graphs bf bf_100.txt -s 42 # optional source override
Assignment 1 — assignment1\
cd C:\iit_ropar_doc\CS509_2026CSM1001\assignment1\matmul_assignment
make                         # build GEMM driver + wrapper
make run-gemm TEST=gemm_test_01.txt
make run-all-gemm
.\common_wrapper\wrapper.exe  # interactive menu
cd C:\iit_ropar_doc\CS509_2026CSM1001\assignment1\csr_assignment
make
make run-one TEST=csr_10.txt
make run-all
Note: these builds originally used MinGW gcc/g++ 4.8.3 with mingw32-make.
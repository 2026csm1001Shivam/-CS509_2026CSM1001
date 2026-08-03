# ============================================================
#  CS509 - Matrix Multiplication assignment repository
#  Top-level Makefile
#
#  Targets:
#    make            build driver + wrapper
#    make driver     build assignment_01 driver
#    make wrapper    build common wrapper menu
#    make run-one TEST=test_01.txt   run one test file
#    make run-all                    run all test files
#    make clean      remove binaries
# ============================================================

CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++11

ASSIGN  = assignment_01
SRC_DIR = $(ASSIGN)/src
DRV_DIR = $(ASSIGN)/driver
TST_DIR = $(ASSIGN)/tests
OUT_DIR = $(ASSIGN)/outputs

DRIVER_BIN  = $(DRV_DIR)/driver
WRAPPER_BIN = common_wrapper/wrapper

all: driver wrapper

driver: $(SRC_DIR)/matmul.cpp $(SRC_DIR)/matmul.h $(DRV_DIR)/driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(SRC_DIR)/matmul.cpp $(DRV_DIR)/driver.cpp -o $(DRIVER_BIN)

wrapper: common_wrapper/wrapper.cpp
	$(CXX) $(CXXFLAGS) -o $(WRAPPER_BIN) common_wrapper/wrapper.cpp

# Run one test file:  make run-one TEST=test_01.txt
run-one: driver
	@if [ ! -f "$(TST_DIR)/$(TEST)" ]; then \
		echo "Error: test file not found: $(TST_DIR)/$(TEST)"; exit 1; \
	fi
	./$(DRIVER_BIN) "$(TST_DIR)/$(TEST)"

run-all: driver
	./$(DRIVER_BIN) --all

clean:
	rm -f $(DRIVER_BIN) $(WRAPPER_BIN)

.PHONY: all driver wrapper run-one run-all clean

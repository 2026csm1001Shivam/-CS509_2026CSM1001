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

driver: $(SRC_DIR)/matrix.cpp $(SRC_DIR)/matrix.h $(SRC_DIR)/gemm.cpp $(SRC_DIR)/gemm.h $(DRV_DIR)/driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(SRC_DIR)/matrix.cpp $(SRC_DIR)/gemm.cpp $(DRV_DIR)/driver.cpp -o $(DRIVER_BIN)

wrapper: common_wrapper/wrapper.cpp
	$(CXX) $(CXXFLAGS) -o $(WRAPPER_BIN) common_wrapper/wrapper.cpp

csr:
	$(MAKE) -C ../csr_assignment

run-gemm: driver
	@if [ ! -f "$(TST_DIR)/$(TEST)" ]; then \
		echo "Error: test file not found: $(TST_DIR)/$(TEST)"; exit 1; \
	fi
	./$(DRIVER_BIN) gemm "$(TST_DIR)/$(TEST)"

run-all-gemm: driver
	./$(DRIVER_BIN) gemm --all

clean:
	rm -f $(DRIVER_BIN) $(WRAPPER_BIN)

.PHONY: all driver wrapper csr run-gemm run-all-gemm clean

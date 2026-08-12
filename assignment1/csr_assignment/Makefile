CXX = g++
CXXFLAGS = -O2 -Wall -Wextra -std=c++11

SRC_DIR = src
DRV_DIR = driver
TST_DIR = tests

DRIVER_BIN = $(DRV_DIR)/driver

all: driver

driver: $(SRC_DIR)/csr.cpp $(SRC_DIR)/csr.h $(DRV_DIR)/driver.cpp
	$(CXX) $(CXXFLAGS) -I$(SRC_DIR) $(SRC_DIR)/csr.cpp $(DRV_DIR)/driver.cpp -o $(DRIVER_BIN)

run-one: driver
	@if [ ! -f "$(TST_DIR)/$(TEST)" ]; then \
		echo "Error: test file not found: $(TST_DIR)/$(TEST)"; exit 1; \
	fi
	./$(DRIVER_BIN) "$(TST_DIR)/$(TEST)"

run-all: driver
	./$(DRIVER_BIN) --all

clean:
	rm -f $(DRIVER_BIN)

.PHONY: all driver run-one run-all clean

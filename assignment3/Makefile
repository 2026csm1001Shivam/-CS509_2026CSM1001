CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
ASSIGN2 = ../assignment2
CFLAGS += -I$(ASSIGN2)

TARGET  = mst
GEN     = gen_mst
OBJS    = main.o mst_graph.o kruskal.o prim.o graph_a2.o
GENOBJS = gen_mst.o

all: $(TARGET) $(GEN)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(GEN): $(GENOBJS)
	$(CC) $(CFLAGS) -o $@ $(GENOBJS)

main.o: main.c $(ASSIGN2)/graph.h $(ASSIGN2)/timer.h mst_graph.h kruskal.h prim.h
	$(CC) $(CFLAGS) -c main.c

mst_graph.o: mst_graph.c mst_graph.h $(ASSIGN2)/graph.h
	$(CC) $(CFLAGS) -c mst_graph.c

kruskal.o: kruskal.c kruskal.h $(ASSIGN2)/graph.h
	$(CC) $(CFLAGS) -c kruskal.c

prim.o: prim.c prim.h kruskal.h $(ASSIGN2)/graph.h
	$(CC) $(CFLAGS) -c prim.c

# CSR conversion function from the previous assignment (Section 3):
# graph.c / AdjListToCSR() is compiled from ../assignment2, NOT copied.
graph_a2.o: $(ASSIGN2)/graph.c $(ASSIGN2)/graph.h
	$(CC) $(CFLAGS) -c $(ASSIGN2)/graph.c -o graph_a2.o

gen_mst.o: gen_mst.c
	$(CC) $(CFLAGS) -c gen_mst.c

ifeq ($(OS),Windows_NT)
RM = del /q
EXE = .exe
else
RM = rm -f
EXE =
endif

run: all
	./$(TARGET)$(EXE)

clean:
	$(RM) $(TARGET)$(EXE) $(GEN)$(EXE) $(OBJS) $(GENOBJS)

.PHONY: all run clean

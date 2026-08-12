CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2
TARGET  = graphs
GEN     = gen_graph
OBJS    = main.o graph.o bellman.o floyd.o
GENOBJS = gen_graph.o graph.o bellman.o

all: $(TARGET) $(GEN)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(GEN): $(GENOBJS)
	$(CC) $(CFLAGS) -o $(GEN) $(GENOBJS)

main.o: main.c graph.h bellman.h floyd.h timer.h
	$(CC) $(CFLAGS) -c main.c

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) -c graph.c

bellman.o: bellman.c bellman.h graph.h
	$(CC) $(CFLAGS) -c bellman.c

floyd.o: floyd.c floyd.h graph.h
	$(CC) $(CFLAGS) -c floyd.c

gen_graph.o: gen_graph.c graph.h bellman.h
	$(CC) $(CFLAGS) -c gen_graph.c

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
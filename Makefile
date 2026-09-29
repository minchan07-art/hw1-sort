# Build and test with one word. Run inside the container (docker compose exec lab bash).
#
#   make run     print the comparison table
#   make test    unit tests
#   make charts  regenerate the charts (SVG) and results CSV under report/
#   make debug   build with debug symbols (used by VS Code F5)
#   make clean   remove build outputs
#
# Executables are named `*.out`; .gitignore filters exactly that.

CC ?= gcc
CFLAGS ?= -std=c17 -Wall -Wextra -O2
DEBUGFLAGS ?= -std=c17 -Wall -Wextra -g -O0
LDLIBS = -lm

.PHONY: all run test charts debug clean

all: test

run: src/main.out
	@./src/main.out

test: tests/test_sort.out
	@./tests/test_sort.out

# Charts are drawn by tools/plot.py using the Python standard library only.
charts: src/main.out
	@python3 tools/plot.py

debug: src/main.debug.out

# Build one file in place, linking every .c in the same folder
# (only one main per folder). Used by the Code Runner button and F5.
%.out: %.c
	$(CC) $(CFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c) $(LDLIBS)

%.debug.out: %.c
	$(CC) $(DEBUGFLAGS) -I$(@D) -o $@ $(wildcard $(@D)/*.c) $(LDLIBS)

SORT_SRC = src/sort.c src/bubbleSort.c src/quickSort.c src/treeSort.c

src/main.out: src/main.c $(SORT_SRC) src/bench.c src/sort.h src/sortctx.h src/bench.h
	$(CC) $(CFLAGS) -Isrc -o $@ src/main.c $(SORT_SRC) src/bench.c $(LDLIBS)

tests/test_sort.out: tests/test_sort.c $(SORT_SRC) src/sort.h src/sortctx.h src/bench.c src/bench.h
	$(CC) $(CFLAGS) -Isrc -o $@ tests/test_sort.c $(SORT_SRC) src/bench.c $(LDLIBS)

clean:
	rm -f src/*.out tests/*.out

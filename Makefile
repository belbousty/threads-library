CC = gcc
CFLAGS+= -D_GNU_SOURCE -g -Wall -Werror #-pthread
TEST_DIR = ./tst
SRC_DIR = ./src
INSTALL_DIR = ./install
LIB_DIR = ${INSTALL_DIR}/lib
BIN_DIR = ${INSTALL_DIR}/bin
LIBS = -lthread

TESTS = 01-main 02-switch 03-equity 11-join 12-join-main 21-create-many 22-create-many-recursive 23-create-many-once 31-switch-many 32-switch-many-join 33-switch-many-cascade 51-fibonacci 52-sum-list 61-mutex 62-mutex 63-mutex-equity 71-preemption 81-deadlock 82-join-same 83-deadlock
all: libthread.so install compile_bin

check:
	./check.sh

valgrind:
	./check.sh valgrind
	

pthreads: CFLAGS += -DUSE_PTHREAD
pthreads: LIBS = -lpthread
pthreads: install compile_bin

static: CFLAGS += -DSTATIC
static: all

graphs:
	python3 graphs.py

install:
	mkdir -p install/bin install/lib
	./cp.sh ./*.so ${LIB_DIR}

compile_bin: ${BIN_DIR}/contextes ${BIN_DIR}/example $(addprefix ${BIN_DIR}/, ${TESTS})

thread.o: ${SRC_DIR}/thread.c
	${CC} ${CFLAGS} -c -fPIC $< -o $@

libthread.so: thread.o
	${CC} ${CFLAGS} $< -shared -o $@

${BIN_DIR}/contextes: ${SRC_DIR}/contextes.c
	${CC} ${CFLAGS} $< -o $@

${BIN_DIR}/example: ${SRC_DIR}/example.c
	${CC} ${CFLAGS} $< -L${LIB_DIR} ${LIBS} -o $@


${BIN_DIR}/%: ${TEST_DIR}/%.c
	${CC} ${CFLAGS} -I${SRC_DIR} $< -L${LIB_DIR} ${LIBS} -o $@

#%: ${TEST_DIR}/%.c
#	${CC} ${CFLAGS} -I${SRC_DIR} $< -L${LIB_DIR} ${LIBS} -o ${BIN_DIR}/$@

clean:
	rm -rf install graphs
	rm -f *.so *.o *.a *.out *.txt

.PHONY: all src install graphs

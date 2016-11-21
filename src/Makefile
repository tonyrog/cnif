CFLAGS = -g

SRCS_CNIF = \
	cnif_lhash.c \
	cnif_io.c \
	cnif_stdio.c \
	cnif_sort.c \
	cnif_copy.c \
	cnif_big.c \
	cnif_misc.c \
	cnif_arith.c \
	cnif.c

SRCS = $(SRCS_CNIF) \
	cnif_test.c \
	cnif_test_big.c

OBJS_TEST = $(SRCS_CNIF:.c=.o) cnif_test.o
OBJS_TEST_BIG = $(SRCS_CNIF:.c=.o) cnif_test_big.o

all: cnif_test cnif_test_big

cnif_test:	$(OBJS_TEST)
	$(CC) -o$@ $(OBJS_TEST)

cnif_test_big:	$(OBJS_TEST_BIG)
	$(CC) -o$@ $(OBJS_TEST_BIG)

-include $(HOME)/make/C.mk

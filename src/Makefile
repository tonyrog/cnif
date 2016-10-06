
CFLAGS = -g
OBJS = \
	cnif_lhash.o \
	cnif_io.o \
	cnif_stdio.o \
	cnif_sort.o \
	cnif_big.o \
	cnif_misc.o \
	cnif.o \
	cnif_test.o

TEST = cnif_test

all: $(TEST)

$(TEST):	$(OBJS)
	$(CC) -o$@ $(OBJS)

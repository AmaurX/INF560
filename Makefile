SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=mpicc
CFLAGS=-O3 -I$(HEADER_DIR) -std=gnu99
LDFLAGS=-lm

SRC= dgif_lib.c \
	egif_lib.c \
	gif_err.c \
	gif_font.c \
	gif_hash.c \
	gifalloc.c \
	main.c \
	tests.c \
	openbsd-reallocarray.c \
	quantize.c \
	gif_utils.c \
	filters.c \
	parallel.c \
	sequential.c \
	role.c

OBJ= $(OBJ_DIR)/dgif_lib.o \
	$(OBJ_DIR)/egif_lib.o \
	$(OBJ_DIR)/gif_err.o \
	$(OBJ_DIR)/gif_font.o \
	$(OBJ_DIR)/gif_hash.o \
	$(OBJ_DIR)/gifalloc.o \
	$(OBJ_DIR)/main.o \
	$(OBJ_DIR)/tests.o \
	$(OBJ_DIR)/openbsd-reallocarray.o \
	$(OBJ_DIR)/quantize.o \
	$(OBJ_DIR)/gif_utils.o \
	$(OBJ_DIR)/filters.o \
	$(OBJ_DIR)/parallel.o \
	$(OBJ_DIR)/sequential.o \
	$(OBJ_DIR)/role.o

all: $(OBJ_DIR) sobelf

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

sobelf:$(OBJ)
	$(CC) $(CFLAGS)  -o $@ $^ $(LDFLAGS)

.PHONY: clean doc test run

clean:
	rm -f sobelf $(OBJ)

doc:
	doxygen doxy/Doxyfile.conf

test: all
	mpirun sobelf 2 images/original/australian-flag-large.gif images/processed/__first.try

#parallel run
run: all
	mpirun sobelf 1 images/original/australian-flag-large.gif images/processed/__first.try
SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj

CC=mpicc
OMP_FLAG = -fopenmp
CFLAGS=-I$(HEADER_DIR) -std=gnu99 -g -Wall

LDFLAGS=-lm

# inutile
# SRC= dgif_lib.c \
# 	egif_lib.c \
# 	gif_err.c \
# 	gif_font.c \
# 	gif_hash.c \
# 	gifalloc.c \
# 	main.c \
# 	tests.c \
# 	openbsd-reallocarray.c \
# 	quantize.c \
# 	gif_utils.c \
# 	filters.c \
# 	parallel.c \
# 	sequential.c \
# 	role.c

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
	$(OBJ_DIR)/role.o\
	$(OBJ_DIR)/structs.o

MAIN_H = $(HEADER_DIR)/main.h

HEADERS =	$(HEADER_DIR)/filters.h \
			$(HEADER_DIR)/gif_hash.h \
			$(HEADER_DIR)/gif_lib_private.h \
			$(HEADER_DIR)/gif_utils.h \
			$(HEADER_DIR)/main.h \
			$(HEADER_DIR)/structs.h \
			$(HEADER_DIR)/tests.h


all: $(OBJ_DIR) sobelf

$(OBJ_DIR):
	mkdir $(OBJ_DIR)

#main_h is a prerequisite as it contains preproc macros
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(MAIN_H)
	$(CC) $(CFLAGS) -c -o $@ $<

sobelf:$(OBJ) $(HEADERS)
	$(CC) $(CFLAGS)  -o $@ $(OBJ) $(LDFLAGS)

.PHONY: clean doc test run

clean:
	rm -f sobelf $(OBJ)

doc:
	doxygen doxy/Doxyfile.conf

test: all
	mpirun sobelf 2 images/original/australian-flag-large.gif images/processed/__first.gif

#parallel runs
massive:
	salloc -N 1 -n 8 mpirun sobelf 1 images/original/TimelyHugeGnu.gif images/processed/__first.gif

run:
	salloc -N 8 -n 8 mpirun sobelf 1 images/original/TimelyHugeGnu.gif images/processed/__first.gif

seq:
	salloc -N 1 -n 1 mpirun sobelf 0 images/original/TimelyHugeGnu.gif images/processed/__first.gif

# local runs
gr-seq:
	mpirun -n 1 sobelf 0 images/original/TimelyHugeGnu.gif images/processed/__first.gif

gr-run: all
	mpirun -n 4 sobelf 1 images/original/TimelyHugeGnu.gif images/processed/__first.gif

gr-ezrun: all
	mpirun -n 8 eztrace -t mpi ./sobelf 1 images/original/TimelyHugeGnu.gif images/processed/__first.gif
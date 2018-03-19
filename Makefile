CC=mpicc
CXX=mpicxx
NVCC=$(shell echo "$(CUDA_ROOT)")/bin/nvcc

export OMPI_CXX=$(NVCC)

export OMPI_CXXFLAGS=-I/users/profs/2017/francois.trahay/soft/install/openmpi-2.1.2/include

export OMPI_LDFLAGS=-I/users/profs/2017/francois.trahay/soft/install/openmpi-2.1.2/include -L/users/profs/2017/francois.trahay/soft/install/hwloc/lib -Xlinker=-rpath -Xlinker=/users/profs/2017/francois.trahay/soft/install/hwloc/lib -Xlinker=-rpath -Xlinker=/users/profs/2017/francois.trahay/soft/install/openmpi-2.1.2/lib -Xlinker=--enable-new-dtags -L/users/profs/2017/francois.trahay/soft/install/openmpi-2.1.2/lib -lmpi

SRC_DIR=src
HEADER_DIR=include
OBJ_DIR=obj
OMP_FLAG = -fopenmp
BUILD_OPTIONS=
CFLAGS=-I$(HEADER_DIR) -g -O3 $(OMP_FLAG) -std=gnu99 -g -Wall $(OMP_FLAG) $(BUILD_OPTIONS)
CUDA_NO_DEPREC=-Wno-deprecated-gpu-targets
CUDAFLAGS=$(CUDA_NO_DEPREC) -g -O3 $(BUILD_OPTIONS)

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
	$(OBJ_DIR)/structs.o\
	$(OBJ_DIR)/cuda_filters.o


MAIN_H = $(HEADER_DIR)/main.h

HEADERS =	$(HEADER_DIR)/filters.h \
			$(HEADER_DIR)/gif_hash.h \
			$(HEADER_DIR)/gif_lib_private.h \
			$(HEADER_DIR)/gif_utils.h \
			$(HEADER_DIR)/main.h \
			$(HEADER_DIR)/structs.h \
			$(HEADER_DIR)/tests.h

.PHONY:truc debug clean test seq massive

# main_h is a prerequisite as it contains preproc macros
# Compilation should therefore be re-done when it changes
$(OBJ_DIR)/cuda_filters.o: $(SRC_DIR)/cuda_filters.cu $(MAIN_H)
	$(CXX) -c -I$(HEADER_DIR),$(HEADER_DIR)/cuda $(CUDAFLAGS) $< -o $@

$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c $(MAIN_H)
	$(CC) $(CFLAGS) -c -o $@ $<

sobelf:clean $(OBJ) $(HEADERS)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS) -lgomp $(CUDA_NO_DEPREC)

debug:
	echo $$OMPI_CXX
	echo $$OMPI_CXXFLAGS
	echo $$OMPI_LDFLAGS
	echo $(NVCC)

clean:
	rm -f sobelf $(OBJ)

doc:
	doxygen doxy/Doxyfile.conf


#######################################################
# RUN rules
#######################################################
IMAGE = totoro.gif
# IMAGE = fire.gif

test: all
	mpirun sobelf 2 images/original/$(IMAGE) images/processed/$(IMAGE)

#parallel runs
massive:
	salloc -N 1 -n 8 mpirun sobelf 1 images/original/$(IMAGE) images/processed/$(IMAGE)

run:
	salloc -N 8 -n 8 mpirun sobelf 1 images/original/$(IMAGE) images/processed/$(IMAGE)

seq:
	salloc -N 1 -n 1 mpirun sobelf 0 images/original/$(IMAGE) images/processed/$(IMAGE)

# local runs
gr-seq:
	mpirun -n 1 sobelf 0 images/original/$(IMAGE) images/processed/$(IMAGE)

gr-run: all
	mpirun -n 8 sobelf 1 images/original/$(IMAGE) images/processed/$(IMAGE)

gr-ezrun: all
	mkdir -p traces/ &&\
	rm -f traces/* &&\
	ls traces &&\
	mpirun -n 8 eztrace -t mpi -o traces ./sobelf 1 images/original/$(IMAGE) \images/processed/$(IMAGE).gif &&\
	eztrace_convert -o traces/out traces/*log* &&\
	vite traces/out.trace
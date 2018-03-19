# INF560
Image processing

Grégoire Roussel & Amaury Camus

3-2018

## Description

The projects aims at developing a image-processing program executing the following sequence of filters:
- a gray filter
- a special blur filter
- a sobel filter

The objective is to make the most of all computing resources available to shorten the processing time. The implementation therefore makes use of the following tools/frameworks:
- MPI
- OpenMP
- Cuda acceleration

## Structure

Source files are locaated into `src` and `include` folders. `include/cuda` contains the set of helper files needed to compile Cuda functions.

## Build & run

NOTE: Before building anything, make sure your env variables are set by :
```sh
. ./set_env.sh
```

The build rules are defined in `Makefile`. The build computer is expected to have the setup of INF560 courses. 
A compatibility Makefile for computer without NVCC is given in `MakeWithoutCuda`. It is non-working but insightful

The `benchmarks` folder contains assets to massively time executions for statical purposes.

The `images/original/` folder contains examples images to run the project on. 



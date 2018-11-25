# Task Dependency Graphs (TDG) Tool

This code is an implementation of a tool that generates task dependency graphs from OpenMP programs.
It works on top of OMPT (TR4) and produces a DOT file as well as various metrics of the graph. It
supports parallel regions and for loops.

In general, we can represent OpenMP code as task dependency graphs in two ways. First, each parallel
region executed by a separate thread is represented as an implicit task. So that if we have *N* threads
we would also have *N* implicit tasks. The parallel loops in this case become part of the implicit task,
without explicit tasks for individual iterations of loops. The second approach would be to represent
each iteration of a parallel loop as a separate task. Although the latter approach provides a more
accurate representation of the parallelism in the code, it would, in most cases, create too much overhead as
there can be potentially many iterations in loops. The solution, therefore, is to combine both of these
approaches and this is what the TDG tool, implemented by this code, tries to do. It creates implicit tasks
for each thread in a parallel region, as well as a task for each chunk in a parallel loop. Since
the chunks can be executed in parallel, explicit breakdown of loops into chunks provides 
a better representation of the parallelism in code.

To generate a TDG with chunks, the Libtdg tool (`libtdg.so`) require a slightly modified version of 
OMPT with a runtime that supports this modification. The new addition adds a new callback called 
*loop-dist* to OMPT and ensures that the runtime invokes this callback before it begins executing 
a new chunk. For now only *dynamic* scheduling of parallel loops is supported. The code changes in 
OMPT and LLVM runtime (TR4) are in a repository called **llvm_omp_tr4**, which is also part of this 
project (Task Graphs Tool).

## Directory contents
* `Makefile` - builds the `libtdg.so` library using ICC and C++11
* `callbacks.{h,cc}` - implementation of OMPT callbacks
* `callbacks_empty.cc` - empty callback implementation for testing OMPT and runtime performance
* `graph.{h,cc}` - code for representing the graph
* `init.cc` - initializes `libtdg.so`
* `init_empty.cc` - empty initialization for testing OMPT and runtime performance
* `metrics.{h,cc}` - code for analyzing the complete TDG, e.g., critical path computation
* `ompt.h` - a copy of OMPT (ver 45) from **llvm-omp-chunks** repository
* `timer` - subdirectory with the code for accurate time measurements
* `test` - a simple test code

## How to build
1. Build the `libftimer.so` (run `make`) in the `timer` subdirectory
2. Run `make`

## Time measurement
The `libftimer.so` is a small library for very fast and accurate time measurements on x86_64
architectures based on the RDTSC instruction. Besides the library itself, running `make` will also
build an executable called `query_cpu_freq`. This executable queries the CPU frequency (cycles
per second) and stores it in file called `.cpu_freq`. The timer initialization in `libftimer.so`
checks if `.cpu_freq` exists, and if it does, the library will read the frequency from this file.
Otherwise it will query the CPU frequency by itself. Since the querying procedure might take a few
seconds, it makes sense to run `query_cpu_freq` before running benchmarks that use `libftimer.so`.

## Libtdg
### Usage
The `test` directory contains a simple example code. Before running the example, be sure to build
the modified LLVM OpenMP runtime in **llvm_omp_tr4** repository. Make the produced `libomp.so` file
visible in `LD_LIBRARY_PATH`. The file `env.sh` provides an example how to do that. Compile the
example (run`make`) and execute it using the line in `run.sh` as an example. Make sure that you place
the full path to the `libtdg.so` file in `LD_PRELOAD`. The example accepts one argument, which is
the number of iterations in the simple parallel for loop it runs.

### Optional metrics
Besides constructing a TDG for the instrumented OpenMP code, the Libtdg tool can also run different
kinds of analysis on the TDG. These are called metrics and for now the tool only provides 3 of them:
* **tim** - computes the total number of tasks and total execution time (i.e., the work); prints the results
to the output
* **cri** - computes the critical path length in terms of execution time and number of tasks and prints
the results to the output
* **dot** - prints the TDG as a DOT file 'tdg.dot'
Any combination of these metrics can be specified in an environment variable called `TDG_TOOL_METRICS`.
For example, `TDG_TOOL_METRICS=tim,dot` or `TDG_TOOL_METRICS=cri`. Note that for codes with many tasks, the
critical path computation is quite slow, so it makes sense to exclude the critical path metric in these
cases.

## TODOs
* Add support for static scheduling (`pragma omp for schedule(static)`)
* Make the critical path computation more efficient

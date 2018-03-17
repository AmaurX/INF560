#/usr/bin/bash

IMAGE=${1:-"totoro.gif"}
OUT="benchmarks/out/"
mkdir -p $OUT
IN="images/original/"

INFILE=$IN$IMAGE
OUTFILE=$OUT$IMAGE

echo "------------------------" 1>&2
echo $INFILE 1>&2
echo "------------------------" 1>&2

function compareRound
{
echo "" 1>&2
echo "======round============" 1>&2
echo "MACHINES : "$1 1>&2
echo "PROCESSES: "$2 1>&2
echo "" 1>&2

ALLOC="salloc -Q -N $1 -n $2 mpirun"

# echo ""
echo "1. sequential" 1>&2
seq=$(timeCommand "benchmarks/sobc0o0 0 $INFILE ${OUT}seq-$IMAGE")

# echo ""
echo "2. MPI onl/y" 1>&2
mpi=$(timeCommand "$ALLOC benchmarks/sobc0o0 1 $INFILE ${OUT}mpi-$IMAGE")

# echo ""
echo "3. MPI+OMP" 1>&2
mpiOmp=$(timeCommand "$ALLOC benchmarks/sobc0o1 1 $INFILE ${OUT}mpiOmp-$IMAGE")

# echo ""
echo "4. MPI+Cuda" 1>&2
mpiCuda=$(timeCommand "$ALLOC benchmarks/sobc1o0 1 $INFILE ${OUT}mpiCuda-$IMAGE")

# echo ""
echo "5. MPI+Cuda+OMP" 1>&2
mpiCuOmp=$(timeCommand "$ALLOC benchmarks/sobc1o1 1 $INFILE ${OUT}mpiCudaOmp-$IMAGE")

echo "********" 1>&2
echo "Seq      $seq" 1>&2
echo "mpi      $mpi" 1>&2
echo "mpiCuda  $mpiCuda" 1>&2
echo "mpiOmp   $mpiOmp" 1>&2
echo "mpiCuOmp $mpiCuOmp" 1>&2
echo "" 1>&2
echo "$IMAGE, $1, $2, $seq, $mpi, $mpiCuda, $mpiOmp, $mpiCuOmp"

}

function timeCommand
{
COMMAND=$1
# echo $COMMAND 1>&2
exec 3>/dev/null 4>&2
var=$( { TIMEFORMAT="%R";time $COMMAND 1>&3 2>&4; } 2>&1 )  # Captures time only.
# var=$( { TIMEFORMAT="%R";time $COMMAND 1>&3 2>&4; } |& tr -d .2>&1 )  # Captures time only.
# echo $var 1>&2
var=$(echo $var |& tr -d . | sed s/^0*//)
# echo $var 1>&2
exec 3>&- 4>&-
echo $var
}

compareRound 1 1
compareRound 4 20
compareRound 20 20
compareRound 40 40

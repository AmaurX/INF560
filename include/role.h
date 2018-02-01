/**
 * role.h
 * Used to attribute role to the processes
 * 
 */

#include <mpi.h>

enum role
{
    master,
    groupmaster,
    slave,
} role;

#define TASK_TAG 0
#define IMAGE_TAG 1

enum role giveRoleInGroup(int rankGroup);

void masterLoop(MPI_Comm *groupCommList, animated_gif *image);

void groupMasterLoop(MPI_Comm groupComm);

void slaveGroup(MPI_Comm groupComm);

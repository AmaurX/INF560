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
};
#define TASK_TAG 0
#define IMAGE_TAG 1

int giveRoleInGroup(int rankGroup);

void masterLoop(MPI_Comm *groupCommList);

void groupMasterLoop(MPI_Comm groupComm);

void slaveGroup(MPI_Comm groupComm);

/**
 * role.h
 * Used to attribute role to the processes
 * 
 */

#include <mpi.h>

#define TASK_TAG 0
#define IMAGE_TAG 1

/// \brief hierarchical position of the process
enum role
{
    master,
    groupmaster,
    slave,
} role;


enum role giveRoleInGroup(int rankGroup);

void masterLoop(MPI_Comm *groupCommList, animated_gif *image);

void groupMasterLoop(MPI_Comm groupComm);

void slaveGroup(MPI_Comm groupComm);

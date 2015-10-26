#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "Semaphore.c"
#include "SharedMemory.c"

#define SEMAPHORE_KEY 64043
// the maximum size of the string of the semid in bytes
#define MAX_ID_SIZE 100
// maximum of 200 characters for now
#define SHM_SIZE 200

/* binary semaphore mutex = 1 
 * nonbinary semaphore wlist = 0
 * int wcount = 0
 * int balance = 500
 * linked-list LIST = NULL
 */

void CatchError(int, char *);

// The identifier for the semaphore group that will be made
int semid;
// The identifier for the shared memory segment that will be made
int shmid;

// The address in memory of the shared memory 
void * memaddr;

// initial starting values of the semaphores
unsigned short counters[] = {0,0,0,0};

// a message to be passed between processes
char * message;

char * pargs[4];

int main() 
{
    pid_t pid = getpid();
    printf("Process ID: %d\n", pid);

    int val;

    // IPC_CREAT signals to make new group if key doesn't already exist
    // CatchError((semid = semget(SEMAPHORE_KEY, 4, IPC_CREAT | ALL_READ_WRITE)), "semget failed\n");
    semid = CreateGroup(SEMAPHORE_KEY, 4, counters);
    printf("Semaphore group with id: %d was just created\n", semid);


    // gets the value of the 0th semaphore in the semid group
    val = GetVal(semid, 0);
    printf("The first semaphore has value %d\n", val);

    val = GetVal(semid, 1);
    printf("The second semaphore has value %d\n", val);

    val = GetVal(semid, 2);
    printf("The third semaphore has value %d\n", val);

    val = GetVal(semid, 3);
    printf("The fourth semaphore has value %d\n", val);


    // Signals the second semaphore (its value should change from 0 to 1)
    Signal(semid, 1);
    printf("The second semaphore has been signaled and has value %d\n", GetVal(semid, 1));

    // creates a new shared memory segment the size of the message
    shmid = CreateSegment((size_t)SHM_SIZE);
    printf("The shared memory segment with id %d has been created\n", shmid);

    memaddr = AttachSegment(shmid);
    printf("The shared memory address at %p has been associated with process %d\n", memaddr, getpid());

    // Assign the arguments for the withdraw program
    pargs[0] = "withdraw";
    pargs[1] = malloc(MAX_ID_SIZE);
    if (snprintf(pargs[1], MAX_ID_SIZE, "%d", semid) < 0)
    {
        perror("snprintf for first argument failed\n");
        exit(EXIT_FAILURE);
    }
    pargs[2] = malloc(MAX_ID_SIZE);
    if (snprintf(pargs[2], MAX_ID_SIZE, "%d", shmid) < 0)
    {
        perror("snprintf for second argument failed\n");
        exit(EXIT_FAILURE);
    }
    pargs[3] = NULL;

    //Create a child process and send it a string
    CatchError((pid = fork()), "fork failed\n");

    if (pid == 0)
    {
        printf("Created new process with pid: %d\n", getpid());
        // the child process is now running the withdraw program 
        CatchError(execvp("./withdraw", pargs), "execvp failed\n");
    }



    printf("Process %d is now waiting for a signal\n", getpid());
    Wait(semid, 0);

    message = "The passed message worked great";

    // j points to the first spot in shared memory
    /*char * j = (char *) memaddr;
    int i;

    // until the end of the string is reached
    for (i = 0; i < strlen(message) + 1; i++)
    {
        // the value of the spot in shared memory equals the value of the current character
        *j = *(message + i);
        // move to next spot in shared memory
        j++;
    }*/
    // This should also tack on the NULL (\0) value to the end of the memaddr
    strcpy(memaddr, message);
    printf("The message should have been added to shared memory\n");


    Signal(semid, 2);
    Wait(semid, 3);

Cleanup:
    free(pargs[1]);
    free(pargs[2]);

    DetachSegment(memaddr);
    printf("The shared memory address at %p assiociated with process %d has been detached\n", memaddr, getpid());

    DestroySegment(shmid);
    printf("The shared memory segment with id %d has been removed\n", shmid);

    DestroyGroup(semid);
    printf("The semaphore group with id %d has been removed\n", semid);

    exit(EXIT_SUCCESS);
}

void CatchError(int x, char * errorMsg)
{
    if (x < 0)
    {
        perror(errorMsg);
        exit(EXIT_FAILURE);
    }
}

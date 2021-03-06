#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>

#include "Semaphore.h"
#include "SharedMemory.h"

#define SEMAPHORE_KEY 64043
#define NUM_SEM 2
#define MUTEX 0
#define FIRST 1
#define SECOND 2
#define THIRD 3


// Pushes the new variables to shared memory
void UpdateSHM();
// Pulls the variables from shared memory
void GetSHM();

unsigned int sleepScale;

char * signature;
char * cssignature;

int semid;
int shmid;
void * memaddr;

unsigned int deposit;

unsigned int wcount;
unsigned int balance;
unsigned int nextWithdraw;
unsigned int processRunning;

unsigned int oldbalance;

// Input: An unsigned int amount that will be added to balance
void main(int argc, char * argv[])
{
    sleepScale = 1;

    signature = malloc(100);
    if (sprintf(signature, "%s%d%s", "--- PID: ", getpid(), " (deposit): ") < 0)
    {
        perror("sprintf failed\n");
        exit(EXIT_FAILURE);
    }

    cssignature = malloc(100);
    if (sprintf(cssignature, "%s%d%s", "*** PID: ", getpid(), " (deposit): ") < 0)
    {
        perror("sprintf failed\n");
        exit(EXIT_FAILURE);
    }

    // Read in arguments
    if (sscanf(argv[1], "%u", &deposit) < 0)
    {
        perror("sscanf failed\n");
        exit(EXIT_FAILURE);
    }

    printf("%sDeposit Process with amount %u has started\n", signature, deposit);
    sleep(2 * sleepScale);

    semid = GetGroup(SEMAPHORE_KEY);

    shmid = GetSegment(SEMAPHORE_KEY);

    memaddr = (void *)AttachSegment(shmid);

    GetSHM();

SemaphoreAlgorithm:

    Wait(semid, MUTEX);

    printf("\n%sEntering Critical Section\n", cssignature);
    sleep(2 * sleepScale);

    GetSHM();

    oldbalance = balance;

    balance = balance + deposit;
    printf("%sDepositing %u\n", cssignature, deposit);
    sleep(2 * sleepScale);

    printf("%s%u + %u = %u\n", cssignature, oldbalance, deposit, balance);
    sleep(2 * sleepScale);

    printf("%sNew Balance = %u\n", cssignature, balance);
    sleep(2 * sleepScale);

    // if there aren't any withdraw processes waiting
    if (wcount == 0)
    {
        printf("%sExiting Critical Section\n\n", cssignature);
        sleep(2 * sleepScale);

        UpdateSHM();

        Signal(semid, MUTEX);
    }
    // if there are withdraw processes waiting
    else 
    {
        // if there is not enough to withdraw now
        if (nextWithdraw > balance)
        {
            printf("%sExiting Critical Section\n\n", cssignature);
            sleep(2 * sleepScale);
 
            UpdateSHM();

            Signal(semid, MUTEX);
        }
        // if there is enough to withdraw
        else
        {
            printf("%sExiting Critical Section\n\n", cssignature);
            sleep(2 * sleepScale);

            UpdateSHM();

            Signal(semid, SECOND);
        }
    }

Cleanup:

    printf("%sDeposit is complete\n", signature);
    sleep(2 * sleepScale);

    free(signature);
    free(cssignature);

    DetachSegment(memaddr);

    exit(EXIT_SUCCESS);
}

void UpdateSHM()
{
    *(unsigned int *)memaddr = wcount;
    *(unsigned int *)(memaddr + sizeof(unsigned int)) = balance;
    *(unsigned int *)(memaddr + 2*sizeof(unsigned int)) = nextWithdraw;
}

void GetSHM()
{
    wcount = *(unsigned int *)memaddr;
    balance = *(unsigned int *)(memaddr + sizeof(unsigned int));
    nextWithdraw = *(unsigned int *)(memaddr + 2*sizeof(unsigned int));
}

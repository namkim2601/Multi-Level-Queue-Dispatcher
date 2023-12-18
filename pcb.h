/* PCB include header file for MLQD dispatcher */

#ifndef MLQD_PCB
#define MLQD_PCB

/* Include files */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* Process Management Definitions *****************************/
#define PCB_UNINITIALIZED 0
#define PCB_INITIALIZED 1
#define PCB_READY 2
#define PCB_RUNNING 3
#define PCB_SUSPENDED 4
#define PCB_TERMINATED 5

// megabytes
#define MEM_LIMIT = 2048 
#define BLOCK_MIN_SIZE = 8

/* Custom Data Types */
struct pcb {
    pid_t pid;
    char * args[3];
    int arrival_time;
    int start_time;
    int service_time;
    int remaining_cpu_time;
    int status;
    int max_iterations;
    int curr_iterations;
    struct mab * mem_block;
    struct pcb * next;
};

typedef struct pcb Pcb;
typedef Pcb * PcbPtr;

/* Function Prototypes */
PcbPtr startPcb(PcbPtr);
PcbPtr suspendPcb(PcbPtr);
PcbPtr terminatePcb(PcbPtr);
PcbPtr printPcb(PcbPtr);
void   printPcbHdr(void);
PcbPtr createnullPcb();
PcbPtr enqPcb(PcbPtr, PcbPtr);
PcbPtr deqPcb(PcbPtr*);

#endif


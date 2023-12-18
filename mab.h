/* MAB include header file for MLQD dispatcher */

#ifndef MLQD_MAB
#define MLQD_MAB

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

// megabytes
#define BLOCK_MIN_SIZE = 8

/* Custom Data Types */
struct mab {
    int offset; // starting address of the memory block
    int size; // size of the memory block
    int allocated; // the block allocated or not
    struct mab * parent; // for use in the Buddy binary tree
    struct mab * left_child; // for use in the binary tree
    struct mab * right_child; // for use in the binary tree
};

typedef struct mab Mab;
typedef Mab * MabPtr;

/* Function Prototypes */
void print_mem_info(MabPtr m); // prints current state of virtual memory
MabPtr memMerge(MabPtr m); // merge buddy memory blocks 
MabPtr memSplit(MabPtr m, int size); // split a memory block
MabPtr memAlloc(MabPtr m, int size); // allocate memory block 
MabPtr memFree(MabPtr m); // free memory block

#endif

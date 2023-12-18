/* PCB management functions for RR dispatcher */

/* Include Files */
#include "pcb.h"

/*******************************************************
 * PcbPtr createnullPcb() - create inactive Pcb.
 *
 * returns:
 *    PcbPtr of newly initialised Pcb
 *    NULL if malloc failed
 ******************************************************/
PcbPtr createnullPcb()
{
    PcbPtr new_process_Ptr;
    if (!(new_process_Ptr = (PcbPtr)malloc(sizeof(Pcb))))
    {
        fprintf(stderr, "ERROR: Could not create new process control block\n");
        return NULL;
    }
    new_process_Ptr->pid = 0;
    new_process_Ptr->args[0] = "./process";
    new_process_Ptr->args[1] = NULL;
    new_process_Ptr->arrival_time = 0;
    new_process_Ptr->service_time = 0;
    new_process_Ptr->remaining_cpu_time = 0;
    new_process_Ptr->status = PCB_UNINITIALIZED;
    new_process_Ptr->next = NULL;
    new_process_Ptr->max_iterations = 0;
    new_process_Ptr->curr_iterations = 0;
    return new_process_Ptr;
}

/*******************************************************
 * PcbPtr enqPcb (PcbPtr headofQ, PcbPtr process)
 *    - queue process (or join queues) at end of queue
 *
 * returns head of queue
 ******************************************************/
PcbPtr enqPcb(PcbPtr q, PcbPtr p)
{
    if (!q)
    {
        return p;
    }
    else
    {
        PcbPtr tmp = q;
        for (; tmp->next != NULL; tmp = tmp->next)
        {
            ;
        }
        tmp->next = p;
        return q;
    }
}

/*******************************************************
 * PcbPtr deqPcb (PcbPtr * headofQ);
 *    - dequeue process - take Pcb from head of queue.
 *
 * returns:
 *    PcbPtr if dequeued,
 *    NULL if queue was empty
 *    & sets new head of Q pointer in adrs at 1st arg
 *******************************************************/
PcbPtr deqPcb(PcbPtr * hPtr)
{
    if (!hPtr || !(*hPtr))
    {
        return NULL;
    }
    else
    {
        PcbPtr tmp = *hPtr;
        *hPtr = (*hPtr)->next;
        tmp->next = NULL;
        return tmp;
    }
}

/*******************************************************
 * PcbPtr startPcb(PcbPtr process) - start (or restart)
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if start (restart) failed
 ******************************************************/
PcbPtr startPcb (PcbPtr p)
{
    if (p->pid == 0)
    {
        switch (p->pid = fork())
        {
            case -1:
                fprintf(stderr, "FATAL: Could not fork process!\n");
                exit(EXIT_FAILURE);
            case 0:
                p->pid = getpid();
                p->status = PCB_RUNNING;
                printPcbHdr();
                printPcb(p);
                fflush(stdout);
                execv(p->args[0], p->args);
                fprintf(stderr, "ALERT: You should never see me!\n");
                exit(EXIT_FAILURE);
        }
    }
    else
    {
        kill(p->pid, SIGCONT);
    }
    p->status = PCB_RUNNING;
    return p;
}

/*******************************************************
 * PcbPtr suspendPcb(PcbPtr process) - suspend
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if suspension failed
 ******************************************************/
PcbPtr suspendPcb(PcbPtr p)
{
    if (!p)
    {
        fprintf(stderr, "ERROR: Cannot suspend a NULL process\n");
        return NULL;
    }
    else if (p->status != PCB_RUNNING)
    {
        fprintf(stderr, "ERROR: Process is not running and cannot be suspended\n");
        return NULL;
    }
    else
    {
        kill(p->pid, SIGTSTP); // Suspend the process with SIGTSTP
        int status;
        if (waitpid(p->pid, &status, WUNTRACED) == -1)
        {
            fprintf(stderr, "ERROR: Failed to wait for process termination\n");
            return NULL;
        }
        p->status = PCB_SUSPENDED;
        return p;
    }
}

/*******************************************************
 * PcbPtr terminatePcb(PcbPtr process) - terminate
 *    a process
 * returns:
 *    PcbPtr of process
 *    NULL if termination failed
 ******************************************************/
PcbPtr terminatePcb(PcbPtr p)
{
    if (!p)
    {
        fprintf(stderr, "ERROR: Cannot terminate a NULL process\n");
        return NULL;
    }
    else
    {
        kill(p->pid, SIGINT); // Terminate the process with SIGINT
        int status;
        if (waitpid(p->pid, &status, 0) == -1)
        {
            fprintf(stderr, "ERROR: Failed to wait for process termination\n");
            return NULL;
        }
        p->status = PCB_TERMINATED;
        return p;
    }
}


/*******************************************************
 * PcbPtr printPcb(PcbPtr process)
 *  - print process attributes on stdout
 *  returns:
 *    PcbPtr of process
 ******************************************************/
PcbPtr printPcb(PcbPtr p)
{
    printf("%7d%7d%7d%7d  ",
        (int) p->pid, p->arrival_time, p->service_time,
            p->remaining_cpu_time);
    switch (p->status) {
        case PCB_UNINITIALIZED:
            printf("UNINITIALIZED");
            break;
        case PCB_INITIALIZED:
            printf("INITIALIZED");
            break;
        case PCB_READY:
            printf("READY");
            break;
        case PCB_RUNNING:
            printf("RUNNING");
            break;
        case PCB_SUSPENDED:
            printf("SUSPENDED");
            break;
        case PCB_TERMINATED:
            printf("PCB_TERMINATED");
            break;
        default:
            printf("UNKNOWN");
    }
    printf("\n");
    
    return p;     
}

/*******************************************************
 * void printPcbHdr() - print header for printPcb
 *  returns:
 *    void
 ******************************************************/
void printPcbHdr()
{  
    printf("    pid arrive  prior    cpu  status\n");

}

/*
    COMP3520 Assignment 2 - MLQD Dispatcher
    SID: 500436282

    usage:
        ./mlqd <TESTFILE>
        where <TESTFILE> is the name of a job list
*/

/* Include files */
#include "pcb.h"
#include "mab.h"

/***    USER FUNCTIONS (to reduce repeated code)    ***/ 

// 1. job_arrival - checks for 'arrival' of incoming jobs
int job_arrival(PcbPtr* job_queue, PcbPtr* arrived_queue, PcbPtr* process, int timer) 
{
    // If a new job has 'arrived'
    if (*job_queue && (*job_queue)->arrival_time <= timer) {
        // Dequeue the ready process from job_queue, and enqueue it to the arrived_queue
        *process = deqPcb(job_queue);
        *arrived_queue = enqPcb(*arrived_queue, *process);
        return 1;
    }
    return 0;
}

// 2. allocate_job - allocate memory for a job before enqueueing it to the Level0_queue
int allocate_job(PcbPtr* arrived_queue, PcbPtr* arrived_head, PcbPtr* level0_queue, MabPtr* first_block, PcbPtr* process) 
{
    if (*arrived_queue || *arrived_head) 
    {
        if (*arrived_head) 
        {
            *process = *arrived_head;
            *arrived_head = NULL;
        } else {
            *process = deqPcb(arrived_queue);
        }

        MabPtr allocated_block = memAlloc(*first_block, (*process)->mem_block->size);
        if (allocated_block) {
            (*process)->mem_block = allocated_block;
            *level0_queue = enqPcb(*level0_queue, *process);
            
            printf("\n");
            print_mem_info(*first_block);

            return 1;
        }

        // Allocation failed - put process back at head of queue
        *arrived_head = *process;
    }
    return 0;
}

// 2. terminate_job - ensures proper termination of a finished job
void terminate_job(PcbPtr* current_process, int timer, double* av_turnaround_time, double* av_wait_time) {
    // A. Terminate the process
    terminatePcb(*current_process);

    // B. Calculate and accumulate turnaround time and wait time
    int turnaround_time = timer - (*current_process)->arrival_time;
    *av_turnaround_time += turnaround_time;
    *av_wait_time += turnaround_time - (*current_process)->service_time;

    // C. Deallocate the PCB's memory
    memFree((*current_process)->mem_block);
    
    free(*current_process);
    *current_process = NULL;
}

/***    MAIN FUNCTION   ***/ 

int main (int argc, char *argv[])
{
    /*** Main function variable declarations ***/
    FILE * input_list_stream = NULL;
    PcbPtr job_queue = NULL;
    PcbPtr arrived_queue = NULL; 
    PcbPtr level0_queue = NULL;
    PcbPtr level1_queue = NULL;
    PcbPtr level2_queue = NULL;

    PcbPtr current_process = NULL;
    PcbPtr process = NULL;

    PcbPtr arrived_head = NULL; // stores the pre-empted process for the Arrived Queue 
    PcbPtr level2_head = NULL; // stores the pre-empted process for the Level-2 Queue 

    // Initialise global memory of 2048 megabytes
    MabPtr first_block = (MabPtr)malloc(sizeof(Mab));
    first_block->offset = (uintptr_t)(malloc(2048)); /* assume this refers to megabytes.
                                        if this were real it would be malloc(2 * 1024^3) */
    first_block->size = 2048;
    first_block->allocated = 0;
    first_block->parent = NULL;
    first_block->left_child = NULL;
    first_block->right_child = NULL;

    int timer = 0;
    int t0; // time quantum for Level-0 queue
    int t1; // time quantum for Level-1 queue
    int k;  // number of iterations for a job to stay in the Level-1 queue
    int quantum; // temp value used to calculate exact time passed for the Level-1 Queue 

    int turnaround_time;
    double av_turnaround_time = 0.0, av_wait_time = 0.0;
    int n = 0;


//  1. Populate the job queue
    if (argc <= 0)
    {
        fprintf(stderr, "FATAL: Bad arguments array\n");
        exit(EXIT_FAILURE);
    }
    else if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <TESTFILE>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!(input_list_stream = fopen(argv[1], "r")))
    {
        fprintf(stderr, "ERROR: Could not open \"%s\"\n", argv[1]);
        exit(EXIT_FAILURE);
    }

    while (!feof(input_list_stream)) {  // put processes into job_queue
        process = createnullPcb();
        process->mem_block = (MabPtr)malloc(sizeof(Mab));
        if (fscanf(input_list_stream,"%d, %d, %d",
             &(process->arrival_time), &(process->service_time), &(process->mem_block->size)) != 3) {
            free(process);
            continue;
        }

        if (process->arrival_time < 0 || process->service_time < 0 ||
            process->mem_block->size < 0 || process->mem_block->size > 2048) {
            fprintf(stderr, "ERROR: Job file %s has invalid entries.\n", argv[1]);
            exit(EXIT_FAILURE);
        }
    
	    process->remaining_cpu_time = process->service_time;
        process->status = PCB_INITIALIZED;
        job_queue = enqPcb(job_queue, process);
	n++;
    }

//  2. Ask the user to specify values for 't0', 't1' and 'k'

    // Input validation for t0 (time quantum for Level-0 queue)
    printf("Please enter a positive integer as the time quantum for the Level-0 queue: ");
    if (scanf("%d", &t0) != 1 || t0 <= 0) {
        fprintf(stderr, "Invalid input. Please enter a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    // Input validation for t1 (time quantum for Level-1 queue)
    printf("Please enter a positive integer as the time quantum for the Level-1 queue: ");
    if (scanf("%d", &t1) != 1 || t1 <= 0) {
        fprintf(stderr, "Invalid input. Please enter a positive integer.\n");
        exit(EXIT_FAILURE);
    }

    // Input validation for k (max number of iterations a job can stay in the Level-1 queue)
    printf("Please enter a positive integer to specify the max number of iterations a job can stay in the Level-1 queue: ");
    if (scanf("%d", &k) != 1 || k <= 0) {
        fprintf(stderr, "Invalid input. Please enter a positive integer.\n");
        exit(EXIT_FAILURE);
    }

//  3. Level-0 Queue: High Priority First-Come-First-Served 
    while (1)
    {   
        
//      i. If there is a currently running process:
        if (current_process)
        {
//          a. Decrement the process's remaining_cpu_time variable;
            current_process->remaining_cpu_time--;
            
//          b. If the process's allocated time has expired:
            if (current_process->remaining_cpu_time <= 0)
                terminate_job(&current_process, timer, &av_turnaround_time, &av_wait_time);

//          c. Else if the process cannot finish within the time-quantum 't0':
            else if ((timer - current_process->start_time) >= t0)
            {
//              A. Suspend the current process and enqueue it to the Level-1 Queue   
                suspendPcb(current_process);
                current_process->max_iterations = k;
                level1_queue = enqPcb(level1_queue, current_process);
                current_process = NULL;
            }
        }

//      ii. Terminate MLQD Dispatcher if it reenters Level-0 with no jobs left to run
        if (!(job_queue || arrived_queue || arrived_head || level0_queue 
        || level1_queue || level2_queue || level2_head || current_process))
            break; 

//      iii. If the next job 'arrives', add it to the Level-0 Queue
        job_arrival(&job_queue, &arrived_queue, &process, timer);

//      iv. If memory can be allocated for the next job in Arrived Queue, 
//              dequeue it from Arrived Queue and add it to Level-0 Queue
        allocate_job(&arrived_queue, &arrived_head, &level0_queue, &first_block, &process);

//      v. If no current_process && Level-0 Queue is not empty:
        if (!current_process && level0_queue)
        {   
//          a. Dequeue a process from the Level-0 Queue, set it as currently running and start it
            current_process = deqPcb(&level0_queue);
            current_process->start_time = timer;
            startPcb(current_process);
        }

//      vi. Else if there are no jobs in the Level-0 Queue
//              and there is a job in the Level-1 Queue:
        else if (!current_process && !level0_queue && level1_queue) 
        {
//          4. Level-1 Queue: Round-Robin
            while (current_process || level1_queue) 
            {
//              i. If a process is currently running:
                if (current_process) 
                { 
//                  a. Decrement process remaining cpu time by quantum(may not be the same as 't1')
                    current_process->remaining_cpu_time -= quantum;

//                  b. Increment number of completed iterations for current process
                    current_process->curr_iterations += 1;

//                  c. If the process's allocated time has expired:
                    if (current_process->remaining_cpu_time <= 0) 
                        terminate_job(&current_process, timer, &av_turnaround_time, &av_wait_time);

//                  d. Else if the current process cannot be completed after k iterations
                    else if (current_process->curr_iterations == current_process->max_iterations)
                    {
//                      A. Suspend the current process and enqueue it to the Level-1 Queue   
                        suspendPcb(current_process);
                        level2_queue = enqPcb(level2_queue, current_process);
                        current_process = NULL;
                    } 

//                  e. Else if the current process is between 0 and k iterations
                    else 
                    {
//                      A. Suspend the current process and enqueue it back Level-1 Queue
                        suspendPcb(current_process);
                        level1_queue = enqPcb(level1_queue, current_process);
                        current_process = NULL;
                    }
                }

//              ii. If the next job 'arrives', add it to the Arrived Queue
                job_arrival(&job_queue, &arrived_queue, &process, timer);

//              iii. If memory can be allocated for the next job in Arrived Queue, 
//                      dequeue it from Arrived Queue and add it to Level-0 Queue
                if (allocate_job(&arrived_queue, &arrived_head, &level0_queue, &first_block, &process))
                    break;

//              iv. If no current_process and Level-1 Queue is empty, either:
//                      a. Instantly terminate MLQD Dispatcher (all jobs finished)
//                      b. go directly to 5. to keep time flow consistent (enters Level-2)
                if (!current_process && !level1_queue)
                    break; // go back to 3.
                
//              v. If no current_process && Level-1 Queue is not empty:
                if (!current_process && level1_queue) 
                {
//                  a. Dequeue process from Level-1 Queue, set it as currently running porcess
                    current_process = deqPcb(&level1_queue);
                    current_process->start_time = timer;
                    
//                  b. If already started but suspended, restart it (send SIGCONT to it)
//                          else start it (fork & exec)
                    startPcb(current_process);
                }
                
//              vi. Sleep for quantum(may not be the same as 't1') and increment dispatcher timer
                quantum = current_process && current_process->remaining_cpu_time < t1 ?
                                current_process->remaining_cpu_time :
                                !(current_process) ? 1 : t1;
                sleep(quantum);
                timer += quantum;
            }
            continue; 
        }

//      vii. Else if there are no jobs in the Level-0 and Level-1 Queues
//           and there is a job in the Level-2 Queue:
        else if (!current_process && !level0_queue && !level1_queue && (level2_queue || level2_head))
        {   
//          5. Level-2 Queue: Low Priority First-Come-First-Served
            while (current_process || level2_head || level2_queue)
            {
//              i. If there is a currently running process:
                if (current_process)
                {
//                  a. Decrement the process's remaining_cpu_time variable;
                    current_process->remaining_cpu_time--;

//                  b. If the process's allocated time has expired:
                    if (current_process->remaining_cpu_time <= 0)
                    {
                        terminate_job(&current_process, timer, &av_turnaround_time, &av_wait_time);
                        level2_head = NULL;
                    }
                }

//              ii. If the next job 'arrives', add it to the Arrived Queue
                job_arrival(&job_queue, &arrived_queue, &process, timer);

//              iii. If memory can be allocated for the next job in Arrived Queue, 
//                      dequeue it from Arrived Queue, add it to Level-0 Queue and run it instantly
                if (allocate_job(&arrived_queue, &arrived_head, &level0_queue, &first_block, &process)) 
                {                
//                  a. Suspend current process and put it at head of Level-2 Queue
                    if (current_process) 
                    {
                        suspendPcb(current_process);
                        level2_head = current_process;
                        current_process = NULL;
                    }
                    break; // go back to 3. 
                }

//              iv. If a job was pre-empted, restart it and set is as currently running
                if (level2_head) 
                {
//                  a. 'Dequeue' process from level2_head, set it as currently running process
                    current_process = level2_head;
                    level2_head = NULL;
//                  b. Process is currently suspended, restart it (send SIGCONT to it) 
                    startPcb(current_process);
                }

//              v. Else if no current_process and Level-2 Queue is empty, either:
//                      a. Instantly terminate MLQD Dispatcher (all jobs finished)
//                      b. Wait for incoming jobs to arrive
                else if (!current_process && !level2_queue)
                    break; // go back to 3. 

//              vi. Else if no current_process && Level-2 Queue is not empty:
                else if (!current_process && level2_queue) 
                {
//                  a. Dequeue process from Level-1 Queue, set it as currently running process
                    current_process = deqPcb(&level2_queue);
                    current_process->start_time = timer;

//                  b. If already started but suspended, restart it (send SIGCONT to it)
//                          else start it (fork & exec)
                    startPcb(current_process);
                }                

//              vii. Sleep for one second and increment dispatcher timer
                sleep(1);
                timer++;
            }
            continue;
        }

//      viii. Sleep for one second and increment dispatcher timer
        sleep(1);
        timer++;

//      go back to 3.
    }

//  6. Print out the total run time, average turnaround time and average wait time
    printf("\ntotal runtime = %i\n", timer);
    av_turnaround_time = av_turnaround_time / n;
    av_wait_time = av_wait_time / n;
    printf("average turnaround time = %f\n", av_turnaround_time);
    printf("average wait time = %f\n", av_wait_time);
    
//  7. Terminate the MLQD dispatcher
    exit(EXIT_SUCCESS);
}
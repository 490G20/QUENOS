/******************************************************************************
NAME:		qcore.c
DESCRIPTION:	The core of the QUERK kernel. Includes the software interrupt
		handler, code to create new processes, and code to perform the
		kernel services.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "qcore.h"
#include "queue.h"
#include "quser.h"
#include "nios2_ctrl_reg_macros.h"

static Process *running_process;

static Queue ready_queue;

extern volatile int* JTAG_UART_ptr = (int*) 0x10001000;// JTAG UART address

static Process process_array[MAX_NUM_OF_PROCESSES];

static char kernel_stack[512];

static int num_of_processes = 0;

static int need_dispatch;

extern unsigned int process_stack_pointer;
extern unsigned int ksp = (unsigned int) &kernel_stack[511];

unsigned int temp_sp;
unsigned int sp_of_first_process;

void put_jtag(volatile int* JTAG_UART_ptr, char c)
{
    while ((*(JTAG_UART_ptr + 1) & 0xFFFF0000) == 0)
        ;
    *(JTAG_UART_ptr) = c;
}

void printString(char *text_string)
{
    int i =0;
    for(i = 0; text_string[i] != 0; ++i){ // print a text string
        put_jtag (JTAG_UART_ptr, text_string[i]);
    }
}

/**
 *  Prints out the ready queue, used for debugging
 */
void ShowReadyQueue (void) {
    Process * p = ready_queue.head;
    printString("queue: ");
    while (p!=0){
            put_jtag(JTAG_UART_ptr,'0'+p->pid);
            p = p->next;
    }
    put_jtag(JTAG_UART_ptr,'\n');
}


/* function to create a new process and add it to the ready queue;
   initial contents of stack are set using stackframe structure */
void QuenosNewProcess (void (*entry_point) (void), char *stack_bottom, int stack_size)
{
    //stackframe pointer deleted but not replaced?
    int new_pid = num_of_processes++;	/* assign new pid */
    Process* new_process = &process_array[new_pid];	/* pointer to descriptor */

    //create new process
    new_process->pid = new_pid;
    new_process->state = READY;
    new_process->user_stack_pointer = stack_bottom + stack_size - 32;
    new_process->program_address = (unsigned int) entry_point;

    // Set ea value in process stack
    unsigned int* p = (unsigned int*) new_process->user_stack_pointer + 29;
    *p = new_process->program_address;

    temp_sp = new_process->user_stack_pointer;

    asm ("movia r12, temp_sp");
    asm ("ldw r13, 0(r12)");
    asm ("stw gp, 104(r13)");

    /* printString("NP \n"); */
    /* showReadyQueue(); */

    AddToTail (&ready_queue, new_process);

    /* showReadyQueue(); */
}

/* functions for kernel services */
static int QuenosCoreBlockSelf (void)
{
    running_process->state = BLOCKED;
    return 1; /* need dispatch of new process */
}

static int QuenosCoreUnblock (int other_pid)
{
    if (process_array[other_pid].state == BLOCKED)
    {
        /* only unblock and add to ready queue if it was blocked */
        process_array[other_pid].state = READY;
        AddToTail (&ready_queue, &process_array[other_pid]);
    }
    return 0; /* no dispatch of new process */
}

/* software interrupt routines */

/* The variables below should ideally be automatic within the two
   following functions, but the compiler adjusts the stack pointer
   at the start/end of each function if there are automatic variables.
   To make sure that we retain control over the stack pointer, the
   variables are made static outside the functions. */

// This is called directly called by the DE2 hardware when a hardware interrupt occurs,
// It is also called indirectly by the_exception(), the software interrupt
void interrupt_handler (void)
{
    /* printString("i\n"); */
    // First task: Update process control block for running process with stackpointer
    running_process->user_stack_pointer = (void*) process_stack_pointer;
    unsigned int* casted_prev_sp = (unsigned int*) running_process->user_stack_pointer;

    int requestType = *(casted_prev_sp+5);

    int ipending;

    /* Third task: retrieve arguments for kernel call. */
    /* They are available on the user process stack. */
    ipending = NIOS2_READ_IPENDING(); // Read the interrupt

    if (ipending) {
    // This currently has no actions, but this will never be called. It will always go to the else since there are no hardware interrupts yet
    }
    else {
            if (requestType == RELINQUISH) {
                /* printString("r\n"); */
                /* showReadyQueue(); */
                running_process->state = READY;
                AddToTail(&ready_queue, running_process);
                need_dispatch = 1;       /* need dispatch of new process */
                /* showReadyQueue(); */
            }
            else if (requestType == BLOCK_SELF) {
                /* printString("blk\n"); */
                /* showReadyQueue(); */
                need_dispatch = QuenosCoreBlockSelf();
                /* showReadyQueue(); */
            }
            else if (requestType == UNBLOCK){
                int other_pid = *(casted_prev_sp+4);
                /* printString("unblk\n"); */
                /* showReadyQueue(); */
                need_dispatch = QuenosCoreUnblock(other_pid);
                /* showReadyQueue(); */
            }

    }

    /* Fifth task: decide if dispatching of new process is needed. */
    if (need_dispatch)
    {
          running_process = DequeueHead(&ready_queue);
          running_process->state = RUNNING;
          /* printString("CP: "); */
          /* put_jtag(JTAG_UART_ptr,'0'+running_process->pid); */
          /* put_jtag(JTAG_UART_ptr,'\n'); */
    }

    process_stack_pointer = (unsigned int) running_process->user_stack_pointer; // This will need to be checked in the debugger
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. It is declared as an interrupt function only
   to generate a return-from-interrupt instruction. */
void QuenosDispatch (void)
{
    /* printString("QD\n"); */
    /* showReadyQueue(); */

    running_process = DequeueHead (&ready_queue);
    running_process->state = RUNNING;

    /* showReadyQueue(); */
    sp_of_first_process = (unsigned int)running_process->user_stack_pointer;

    asm("movia r12, sp_of_first_process");
    asm("ldw sp, 0(r12)");
    asm("ldw ea, 116(sp)");
    asm("addi sp, sp, 128");

    asm("eret");
}


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
#include "qrequest.h"
#include "nios2_ctrl_reg_macros.h"

/*----------------------------------------------------------------*/

static  Process     *running_process;

static  Queue   ready_queue;

static  char    kernel_stack[512];
static  void    *kernel_stack_pointer = (void *) &kernel_stack[511];

static  int     num_of_processes = 0;	/* counter used to assign unique pids */

static  Process     process_array[MAX_NUM_OF_PROCESSES]; // Formerly pdb_array

/*
List of all registers:

R0 zero
R1
R2 assembler temporary
R3 return value (least-significant 32 bits)
R4 return value (most significant 32 bits)
R5 register argument 1st 32 bits
R6 ""       ""       2nd
R7 3rd 32 bits
R8 4th 32 bits

Caller saved general purpose registers
R9
R10
R11
R12
R13
R14
R15

Callee saved general purpose registers
R16
R17
R18
R19
R20
R21

Different on a linux system
R22
R23

24et  - Exception Temporary (NOT AVAILABLE IN USER MODE)
25bt	- Breakpoint Temporary (ONLY IN JTAG DEBUG MODULE)
26gp	- Global Pointer
27sp	- Stack Pointer
28fp	- Frame Pointer
29ea	- Exception Return Address (NOT AVAILABLE IN USER MODE)
30ba	- Breakpoint Return Address (ONLY IN JTAG DEBUG MODULE)
31ra	- Return Address PC/ Program counter

*/

/*----------------------------------------------------------------*/
/* function to create a new process and add it to the ready queue;
   initial contents of stack are set using stackframe structure */

void    QuenosNewProcess (void (*entry_point) (void), char *stack_bottom,
                         int stack_size)
{
		//stackframe pointer deleted but not replaced? 
        int             new_pid = num_of_processes++;	/* assign new pid */
        Process             *new_process; // Formerly pdb

        //create new process
        new_process		= &process_array[new_pid];	/* pointer to descriptor */
        new_process->pid	= new_pid;
        new_process->state	= Ready;
        new_process->user_stack_pointer		= stack_bottom + stack_size; // XXX: Confirm that this is correct

        AddToTail (&ready_queue, new_process);
}

/*----------------------------------------------------------------*/
/* functions for kernel services */

static  int     QuenosCoreBlockSelf (void)
{
        running_process->state = Blocked;
        return 1;       /* need dispatch of new process */
}

static  int     QuenosCoreUnblock (int other_pid)
{
	if (process_array[other_pid].state == Blocked)
	{
		/* only unblock and add to ready queue if it was blocked */
	        process_array[other_pid].state = Ready;
		AddToTail (&ready_queue, &process_array[other_pid]);
	}
        return 0;       /* no dispatch of new process */
}

/*----------------------------------------------------------------*/
/* software interrupt routines */

/* The variables below should ideally be automatic within the two
   following functions, but the compiler adjusts the stack pointer
   at the start/end of each function if there are automatic variables.
   To make sure that we retain control over the stack pointer, the
   variables are made static outside the functions. */

//TODO: remove and refactor as we don't want to asm directives
/**
	Expect kernel_stack_pointer_address to magically get into register 4, which we will then write into the stackpoiter
*/
void writeRegisterValueToSP (void* registerValue){
    asm("mov sp, r4");
}

static  int     need_dispatch;
static  Request request;

// This is called directly called by the DE2 hardware when a hardware interrupt occurs,
// It is also called indirectly by the_exception(), the software interrupt
void    interrupt_handler (void) //TODO: if we must move interrupt handler to separate file, then various interactions, how to adapt?
{
    //TODO: SAVE CURRENTLY RUNNING PROCESS STACK POINTER HERE
        //? switch to kernel stack?
    // TODO: change all assembly to make sense
    /* Use embedded assembly language to copy SP in D, then into PDB. */
    //running_process->user_stack_pointer = (void *) asm("mov sp,d"); // Save the stack pointer to the static_stackframe_pointer

    //register int sp asm("sp");
    //running_process->user_stack_pointer = sp; // Pointer from integer no casting warning

    //writeRegisterValueToSP(kernel_stack_pointer);
    //TODO: update to use kernel stack pointer address into register sp? could we hard code

	// ISR
	register int other_pid asm("r22");
	register int requestType asm("r23");
	int ipending;

    /* Third task: retrieve arguments for kernel call. */
    /* They are available on the user process stack. */
	ipending = NIOS2_READ_IPENDING(); // Read the interrupt

	if (ipending) {
		// This currently has no actions, but this will never be called. It will always go to the else since there are no hardware interrupts yet
	}
	else {
		if (requestType == Relinquish) {
			running_process->state = Ready;
			AddToTail(&ready_queue, running_process);
			need_dispatch = 1;       /* need dispatch of new process */
		}
		else if (requestType == BlockSelf) {
			need_dispatch = QuenosCoreBlockSelf();
		}
		else if (requestType == Unblock){
			need_dispatch = QuenosCoreUnblock(other_pid);
		}
		/* Fifth task: decide if dispatching of new process is needed. */
		if (need_dispatch)
		{
			running_process = DequeueHead(&ready_queue);
			running_process->state = Running;
		}
	}
    // ISR ____________

    //TODO: we must update running process stack pointer here, with the new thing running
    /* Sixth task: switch back to user stack pointer and return */

    // Write the running->user_stack_pointer sp value we saved when saving context back to register sp

    //	writeRegisterValueToSP(running_process->user_stack_pointer);

//    kernel_stack_pointer = (void *) asm("mov sp,d"); //get d and put it into sp
//    asm("mov d,sp", running_process->user_stack_pointer);

    // Do we want to keep this or is it just redundant?
    //register int sp asm("sp");
    //running_process->user_stack_pointer = sp;
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. It is declared as an interrupt function only
   to generate a return-from-interrupt instruction. */

//@interrupt	void    QuenosDispatch (void)
	void    QuenosDispatch (void)

{
        running_process = DequeueHead (&ready_queue);
        running_process->state = Running;
		
		// TODO: change all assembly to make sense
		writeRegisterValueToSP(running_process->user_stack_pointer);

        //asm("mov d,sp", running_process->user_stack_pointer); /* sets SP to user stack */

        //todo: how to generate a return from interrupt instruction from here for nios 2
        /* compiler generates a return-from-interrupt instruction here. */
}

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

volatile int* JTAG_UART_ptr = (int*) 0x10001000;// JTAG UART address

static  char    kernel_stack[512];
static  void    *kernel_stack_pointer = (void *) &kernel_stack[511];
// Ask why we can't just define this in the exception handler and

static  int     num_of_processes = 0;	/* counter used to assign unique pids */

static  Process     process_array[MAX_NUM_OF_PROCESSES]; // Formerly pdb_array
extern unsigned int process_stack_pointer;
extern unsigned int ksp;

void	put_jtag( volatile int* JTAG_UART_ptr, char c )
{
	int control;
	control = *(JTAG_UART_ptr + 1);	// read the JTAG_UART Control register

	if(control & 0xFFFF0000) {// if space, then echo character, else ignore
		*(JTAG_UART_ptr) = c;
	}
}

void printString(char text_string[] )
{
    int i =0;
    for(i = 0; text_string[i] != 0; ++i){ // print a text string
        put_jtag (JTAG_UART_ptr, text_string[i]);
    }
}

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
// Assumption this is meant to mock out the stack we would've saved in exception_handler.c
typedef struct _stackframe
{
	// TODO: get peer review on math, offsets and testing
	unsigned int //r24 et, 25 bt, 26 gp, 27 is sp and not saved, 28 fp, 29 ea, 30 ba, 31 ra
			r1,r2,r3,r4,
			r5,r6,r7,r8,
			r9,r10,r11,r12,
			r13,r14,r15,r16,
			r17,r18,r19,r20,
			r21,r22,r23,et,
			bt,gp

			//r27 is sp
			,fp,ea,ba,ra;

} StackFrame;

/**
 * Hardcoded new process that adds itself to the ready queue used to fake out initial history?
 * initial contents of stack are set using the stackframe structure
 *
 * @param entry_point
 * @param stack_bottom
 * @param stack_size
 */
void    QuenosNewProcess (void (*entry_point) (void), char *stack_bottom,
                         int stack_size)
{
		StackFrame *sfp;
		//stackframe pointer deleted but not replaced?
        int             new_pid = num_of_processes++;	/* assign new pid */
        Process             *new_process; // Formerly pdb

        printString("New process created pid : ");
        printString(new_pid);
        printString("\n>\0");

   		 //create new process
        new_process		= &process_array[new_pid];	/* pointer to descriptor */
        new_process->pid	= new_pid;
        new_process->state	= Ready;
        new_process->user_stack_pointer		= stack_bottom + stack_size - sizeof(StackFrame); // XXX: Confirm that this is correct

		// What are the parts of the stack frame we care about and will interact with?
			// We must preload ea with the correct PC value
			// Check if we need to preload correct value of estatus here
		sfp->ea = (unsigned int) entry_point;
	
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

static  int     need_dispatch;
//static  Request request; //TODO: remove

// This is called directly called by the DE2 hardware when a hardware interrupt occurs,
// It is also called indirectly by the_exception(), the software interrupt
void    interrupt_handler (void)
{

	// First task: Update process control block for running process with stackpointer
	running_process->user_stack_pointer = (void*) process_stack_pointer;
	unsigned int* casted_prev_sp = (unsigned int*) running_process->user_stack_pointer;

    //TODO: SAVE CURRENTLY RUNNING PROCESS STACK POINTER HERE
    //  UPDATE TO KERNEL STACK POINTER
    //TODO: Why the  +1 in below?
    int *dummy_address = (int *)running_process->user_stack_pointer + 1;
    dummy_address = kernel_stack_pointer;

	int requestType = *(casted_prev_sp+5);

	int ipending;

    /* Third task: retrieve arguments for kernel call. */
    /* They are available on the user process stack. */
	ipending = NIOS2_READ_IPENDING(); // Read the interrupt

	// TODO: read in request type from kernel? stack at offset 20(sp)

	if (ipending) {
		// This currently has no actions, but this will never be called. It will always go to the else since there are no hardware interrupts yet
	}
	else {
		if (requestType == 1) {
			printString("\nRelinquish\n>\0");
			running_process->state = Ready;
			AddToTail(&ready_queue, running_process);
			need_dispatch = 1;       /* need dispatch of new process */
		}
		else if (requestType == 2) {
			printString("\nBlockSelf\n>\0");
			need_dispatch = QuenosCoreBlockSelf();
		}
		else if (requestType == 3){
			printString("\nUnblock\n>\0");
			int other_pid = *(casted_prev_sp+4);
			need_dispatch = QuenosCoreUnblock(other_pid);
		}

	}

	/* Fifth task: decide if dispatching of new process is needed. */
	if (need_dispatch)
	{
		running_process = DequeueHead(&ready_queue);
		running_process->state = Running;
	}

    // ISR ____________

    //TODO: we must update running process stack pointer here, with the new thing running
    /* Sixth task: switch back to user stack pointer and return */
	kernel_stack_pointer = &kernel_stack[511];
	process_stack_pointer = (unsigned int) running_process->user_stack_pointer; // This will need to be checked in the debugger
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. It is declared as an interrupt function only
   to generate a return-from-interrupt instruction. */

//@interrupt	void    QuenosDispatch (void)
void    QuenosDispatch (void)
{
		printString("\nDispatch\n>\0");
        running_process = DequeueHead (&ready_queue);
        running_process->state = Running;
		
		// TODO: change all assembly to make sense

        //todo: how to generate a return from interrupt instruction from here for nios 2
        /* compiler generates a return-from-interrupt instruction here. */
        printf("now dispatching ");
        printf(running_process->pid);
        printf("\n");
        process_stack_pointer = running_process->user_stack_pointer;

        asm("eret");    //TODO: to force fompiler to generate a return from interrupt instruction here is asm directive use ok here?
}


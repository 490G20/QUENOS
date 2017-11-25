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

/**
	Expect kernel_stack_pointer_address to magically get into register 4, which we will then write into the stackpoiter
*/
void writeRegisterValueToSP (void* registerValue){
    asm("mov sp, r4");
}

static  int     need_dispatch;
static  Request request;

void QuenosSaveContext (void) {
    asm(".set noat"); // magic, for the C compiler
    asm(".set nobreak"); // magic, for the C compiler
    asm("subi sp, sp, 128");
    asm("stw et, 96(sp)");
    asm("rdctl et, ctl4");
    asm("beq et, r0, SKIP_EA_DEC"); // interrupt is not external
    asm("subi ea, ea, 4"); /* must decrement ea by one instruction for external
							* interrupts, so that the instruction will be run */

    asm("SKIP_EA_DEC:");
    asm("stw r1, 4(sp)"); // save all registers
    asm("stw r2, 8(sp)");
    asm("stw r3, 12(sp)");
    asm("stw r4, 16(sp)");
    asm("stw r5, 20(sp)");
    asm("stw r6, 24(sp)");
    asm("stw r7, 28(sp)");
    asm("stw r8, 32(sp)");
    asm("stw r9, 36(sp)");
    asm("stw r10, 40(sp)");
    asm("stw r11, 44(sp)");
    asm("stw r12, 48(sp)");
    asm("stw r13, 52(sp)");
    asm("stw r14, 56(sp)");
    asm("stw r15, 60(sp)");
    asm("stw r16, 64(sp)");
    asm("stw r17, 68(sp)");
    asm("stw r18, 72(sp)");
    asm("stw r19, 76(sp)");
    asm("stw r20, 80(sp)");
    asm("stw r21, 84(sp)");
    asm("stw r22, 88(sp)");
    asm("stw r23, 92(sp)");
    asm("stw r25, 100(sp)"); // r25 = bt (skip r24 = et, because it was saved above)
    asm("stw r26, 104(sp)"); // r26 = gp
    asm("stw r27, 108(sp)"); // r27 = sp
    asm("stw r28, 112(sp)"); // r28 = fp
    asm("stw r29, 116(sp)"); // r29 = ea
    asm("stw r30, 120(sp)"); // r30 = ba
    asm("stw r31, 124(sp)"); // r31 = ra
    asm("addi fp, sp, 128");
	
	// TODO: change all assembly to make sense
	/* Use embedded assembly language to copy SP in D, then into PDB. */
    //running_process->user_stack_pointer = (void *) asm("mov sp,d"); // Save the stack pointer to the static_stackframe_pointer
    register int sp asm("sp");
    running_process->user_stack_pointer = sp; // Pointer from integer no casting warning
	
    /* Second task: switch to kernel stack by modifying stack pointer. */
    //asm("mov d,sp", kernel_stack_pointer); //assumption dont need to shove sp into register d to read in, can just do below
	
	// Type mismatch warning 
	writeRegisterValueToSP(kernel_stack_pointer);
	//write in kernel stack pointer address into register sp? could we hard code this into an assembly language file somewhere 274, 371 style?
	
}

void QuenosRestoreContext(void) {

    /* Sixth task: switch back to user stack pointer and return */
	//Write the running->user_stack_pointer sp value we saved when saving context back to register sp
	writeRegisterValueToSP(running_process->user_stack_pointer);

    // TODO: change all assembly to make sense
    //kernel_stack_pointer = (void *) asm("mov sp,d"); //get d and put it into sp
    //asm("mov d,sp", running_process->user_stack_pointer); 
	
	// Do we want to keep this or is it just redundant?
	//register int sp asm("sp");
    //running_process->user_stack_pointer = sp;

    asm("ldw r1, 4(sp)"); // restore all registers
    asm("ldw r2, 8(sp)");
    asm("ldw r3, 12(sp)");
    asm("ldw r4, 16(sp)");
    asm("ldw r5, 20(sp)");
    asm("ldw r6, 24(sp)");
    asm("ldw r7, 28(sp)");
    asm("ldw r8, 32(sp)");
    asm("ldw r9, 36(sp)");
    asm("ldw r10, 40(sp)");
    asm("ldw r11, 44(sp)");
    asm("ldw r12, 48(sp)");
    asm("ldw r13, 52(sp)");
    asm("ldw r14, 56(sp)");
    asm("ldw r15, 60(sp)");
    asm("ldw r16, 64(sp)");
    asm("ldw r17, 68(sp)");
    asm("ldw r18, 72(sp)");
    asm("ldw r19, 76(sp)");
    asm("ldw r20, 80(sp)");
    asm("ldw r21, 84(sp)");
    asm("ldw r22, 88(sp)");
    asm("ldw r23, 92(sp)");
    asm("ldw r24, 96(sp)");
    asm("ldw r25, 100(sp)"); // r25 = bt
    asm("ldw r26, 104(sp)"); // r26 = gp
    // skip r27 because it is sp, and we did not save this on the stack
    asm("ldw r28, 112(sp)"); // r28 = fp
    asm("ldw r29, 116(sp)"); // r29 = ea
    asm("ldw r30, 120(sp)"); // r30 = ba
    asm("ldw r31, 124(sp)"); // r31 = ra
    asm("addi sp, sp, 128");
    asm("eret");
}

// This is called directly called by the DE2 hardware when a hardware interrupt occurs,
// It is also called indirectly by the_exception(), the software interrupt
void    interrupt_handler (void)
{
    /* On entry, all user registers must be saved to the process stack. */
	QuenosSaveContext();

	register int other_pid asm("r22");
	register int requestType asm("r23");
	int ipending;

    /* Third task: retrieve arguments for kernel call. */
    /* They are available on the user process stack. */
	NIOS2_READ_IPENDING(ipending); // Read the interrupt

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
    /* compiler generates a return-from-interrupt instruction here. */
	QuenosRestoreContext();
}

void the_exception(void) __attribute__ ((section (".exceptions")));
void the_exception(void) {
	interrupt_handler();
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

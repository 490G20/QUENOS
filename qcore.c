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

/* This structure is used to set the initial contents of the stack
   when creating a new process, and also to retrieve parameters on
   entry to the software interrupt handler. */

typedef struct  _stackframe //all the registers
{
    //TODO: investigate if architecture specific and review 274
    /* the order is important here... */
        unsigned char   CCR,	//condition code register
                        B, // SWI register containing the process calling the interrupt
                        A,	//SWI Register holding the type of interrupt (In this program 000,001,010)
							// Also, this turns out to be th upper 4 bits of D, whcih is why we use ADDD #0 in qrequest
                        Xhi, //register x hi __ bits
                        Xlo,
                        Yhi,
                        Ylo,
                        PChi, //program counter
                        PClo;
} StackFrame;

/*
List of all registers:

R0 zero
R1
R2
R3
R4
R5
R6
R7
R8
R9
R10
R11
R12
R13
R14
R15
R16
R17
R18
R19
R20
R21
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

void    QuerkNewProcess (void (*entry_point) (void), char *stack_bottom,
                         int stack_size)
{
        //StackFrame      *stackframe_pointer; // Formerly sfp
        int             new_pid = num_of_processes++;	/* assign new pid */
        Process             *new_process; // Formerly pdb

        //create new process
        new_process		= &process_array[new_pid];	/* pointer to descriptor */
        new_process->pid	= new_pid;
        new_process->state	= Ready;
        new_process->user_stack_pointer		= stack_bottom + stack_size - sizeof (StackFrame);

        //update the stack frame pointer
        stackframe_pointer		= (StackFrame *) new_process->user_stack_pointer;

        //may need to change based on architecture/
		//stackframe_pointer->PChi	= (unsigned char) (((int) entry_point & 0xff00) >> 8);
        //stackframe_pointer->PClo	= (unsigned char) (((int) entry_point & 0x00ff));
        //stackframe_pointer->CCR	= (unsigned char) 0xc0; /* 11000000 in binary */

        AddToTail (&ready_queue, new_process);
}

/*----------------------------------------------------------------*/
/* functions for kernel services */

static  int     QuerkCoreBlockSelf (void)
{
        running_process->state = Blocked;
        return 1;       /* need dispatch of new process */
}

static  int     QuerkCoreUnblock (int other_pid)
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
static  Request request;
static  int     other_pid;
static  StackFrame      *static_stackframe_pointer; //TODO: refactor again later when better understood

//review later
//@interrupt      void    QuerkSWIHandler (void)
void    interrupt_handler (void)
{
	int ipending;
    /* On entry, all user registers must be saved to the process stack. */
	QuerkSaveContext();

   
    /* Third task: retrieve arguments for kernel call. */
    /* They are available on the user process stack. */
	NIOS2_READ_IPENDING(ipending); // Read the interrupt

    //TODO: Set other_pid
	if (ipending & 0x1) { //Software interrupt for Relinquish
		//TODO: replace value to and ipending
		running_process->state = Ready;
		AddToTail(&ready_queue, running_process);
		need_dispatch = 1;       /* need dispatch of new process */
	}
	if (ipending & 0x2) { //Software interrupt for BlockSelf
		//TODO: replace value to AND ipending for the software interrupt bit
		need_dispatch = QuerkCoreBlockSelf();
	}
	if (ipending & 0x4) { //Software interrupt for QuerkCoreUnblock
		//TODO: replace value to and ipending
		need_dispatch = QuerkCoreUnblock(other_pid);
	}
    /* Fifth task: decide if dispatching of new process is needed. */
    if (need_dispatch)
    {
            running_process = DequeueHead (&ready_queue);
            running_process->state = Running;
    }

		
        
    /* compiler generates a return-from-interrupt instruction here. */
	QuerkRestoreContext();
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. It is declared as an interrupt function only
   to generate a return-from-interrupt instruction. */

//@interrupt	void    QuerkDispatch (void)
	void    QuerkDispatch (void)

{
        running_process = DequeueHead (&ready_queue);
        running_process->state = Running;
		// TODO: change all assembly to make sense
        _asm ("tfr d,sp", running_process->user_stack_pointer); /* sets SP to user stack */

        //todo: how to generate a return from interrupt instruction from here for nios 2
        /* compiler generates a return-from-interrupt instruction here. */
}

void QuerkSaveContext (void) {
{
	asm(".set noat"); // magic, for the C compiler
	asm(".set nobreak"); // magic, for the C compiler
	asm("subi sp, sp, 128");
	asm("stw et, 96(sp)");
	asm("rdctl et, ctl4");
	asm("beq et, r0, SKIP_EA_DEC"); // interrupt is not external
	asm("subi ea, ea, 4"); /* must decrement ea by one instruction for external
							* interrupts, so that the instruction will be run */

    register int generalRegister22Contents asm("r22");
    other_pid = generalRegister22Contents;

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

	/* Use embedded assembly language to copy SP in D, then into PDB. */
	// TODO: change all assembly to make sense

	running_process->user_stack_pointer = (void *) _asm ("tfr sp,d"); // Save the stack pointer to the static_stackframe_pointer
	static_stackframe_pointer = (StackFrame *)running_process->user_stack_pointer;

	/* Second task: switch to kernel stack by modifying stack pointer. */
	_asm ("tfr d,sp", kernel_stack_pointer);
	register int sp asm("sp");
	running_process->user_stack_pointer = sp;
}

void QuerkRestoreContext(void) {

	/* Sixth task: switch back to user stack pointer and return */
	// TODO: change all assembly to make sense
	kernel_stack_pointer = (void *) _asm ("tfr sp,d");
	_asm ("tfr d,sp", running_process->user_stack_pointer);

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


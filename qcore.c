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

/*----------------------------------------------------------------*/

static  PDB     *running_pdb;

static  Queue   ready_queue;

static  char    kernel_stack[512];
static  void    *kernel_SP = (void *) &kernel_stack[511];

static  int     num_pdb = 0;	/* counter used to assign unique pids */

static  PDB     pdb_array[MAX_PDB];

/* This structure is used to set the initial contents of the stack
   when creating a new process, and also to retrieve parameters on
   entry to the software interrupt handler. */

typedef struct  _stackframe
{
        unsigned char   CCR,	/* the order is important here... */
                        B,
                        A,
                        Xhi,
                        Xlo,
                        Yhi,
                        Ylo,
                        PChi,
                        PClo;
} StackFrame;

/*----------------------------------------------------------------*/
/* function to create a new process and add it to the ready queue;
   initial contents of stack are set using stackframe structure */

void    QuerkNewProcess (void (*entry_point) (void), char *stack_bottom,
                         int stack_size)
{
        StackFrame      *sfp;
        int             new_pid = num_pdb++;	/* assign new pid */
        PDB             *pdb;

        pdb		= &pdb_array[new_pid];	/* pointer to descriptor */
        pdb->pid	= new_pid;
        pdb->state	= Ready;
        pdb->SP		= stack_bottom + stack_size - sizeof (StackFrame);
        sfp		= (StackFrame *) pdb->SP;
        sfp->PChi	= (unsigned char) (((int) entry_point & 0xff00) >> 8);
        sfp->PClo	= (unsigned char) (((int) entry_point & 0x00ff));
        sfp->CCR	= (unsigned char) 0xc0; /* 11000000 in binary */

        AddToTail (&ready_queue, pdb);
}
        
/*----------------------------------------------------------------*/
/* functions for kernel services */

static  int     QuerkCoreBlockSelf (void)
{
        running_pdb->state = Blocked;
        return 1;       /* need dispatch of new process */
}

static  int     QuerkCoreUnblock (int other_pid)
{
	if (pdb_array[other_pid].state == Blocked)
	{
		/* only unblock and add to ready queue if it was blocked */
	        pdb_array[other_pid].state = Ready;
		AddToTail (&ready_queue, &pdb_array[other_pid]);
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
static  StackFrame      *sfp;


@interrupt      void    QuerkSWIHandler (void)
{
        /* On entry, all user registers have been saved on process stack. */

	/* First task: save user stack pointer in process control block. */
        /* Use embedded assembly language to copy SP in D, then into PDB. */
        running_pdb->SP	= (void *) _asm ("tfr sp,d");
        sfp		= (StackFrame *) running_pdb->SP;

        /* Second task: switch to kernel stack by modifying stack pointer. */
        _asm ("tfr d,sp", kernel_SP);


        /* Third task: retrieve arguments for kernel call. */
        /* They are available on the user process stack. */
        request	  = (Request) sfp->A;
        other_pid = sfp->B;

        /* Fourth task: invoke appropriate routine for request. */
        switch (request)
        {
            case Relinquish:
                running_pdb->state = Ready;
                AddToTail (&ready_queue, running_pdb);
                need_dispatch = 1;       /* need dispatch of new process */
                break;
            case BlockSelf:
                need_dispatch = QuerkCoreBlockSelf ();
                break;
            case Unblock:
                need_dispatch = QuerkCoreUnblock (other_pid);
                break;
        }

        /* Fifth task: decide if dispatching of new process is needed. */
        if (need_dispatch)
        {
                running_pdb = DequeueHead (&ready_queue);
                running_pdb->state = Running;
        }

        /* Sixth task: switch back to user stack pointer and return */
        kernel_SP = (void *) _asm ("tfr sp,d");
        _asm ("tfr d,sp", running_pdb->SP);

        /* compiler generates a return-from-interrupt instruction here. */
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. It is declared as an interrupt function only
   to generate a return-from-interrupt instruction. */

@interrupt	void    QuerkDispatch (void)
{
        running_pdb = DequeueHead (&ready_queue);
        running_pdb->state = Running;
        _asm ("tfr d,sp", running_pdb->SP); /* sets SP to user stack */

        /* compiler generates a return-from-interrupt instruction here. */
}

/******************************************************************************
NAME:		qmain.c
DESCRIPTION:	Startup code for the QUERK kernel. Sets software interrupt
		vector, creates null process, calls user-supplied function
		to create user processes, then selects first ready process
		to run.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "qinit.h"
#include "qcore.h"
#include "qrequest.h"
#include "quser.h"

/*----------------------------------------------------------------*/
#define cli()	asm("andcc #$EF\n")	/* clear Interrupt? bit in CCR */ //TODO: change for altera architecture

#define NULL_PROCESS_STACK_SIZE 256 // TODO: confirm is same for altera nios 2, if not update

static  char    NullProcessStack[NULL_PROCESS_STACK_SIZE];

/*----------------------------------------------------------------*/

void    NullProcess (void)
{
	QuenosRelinquish ();	/* null process simply surrenders processor */
}

int     main ()
{
        /* initialize interrupt vectors and any other things... */
        QuenosInit ();

        /* create null process and add to ready queue */
        QuenosNewProcess (NullProcess, NullProcessStack,
                         NULL_PROCESS_STACK_SIZE);

        /* create user processes and add to ready queue */
        UserProcesses ();

        /* enable all interrupts */
        cli();

        /* start up the first process (we never return here) */
        QuenosDispatch ();
}

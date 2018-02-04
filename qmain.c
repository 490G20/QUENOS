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
#include "quser.h"
#include <string.h>

#define PROCESS_STACK_SIZE 256

static char NullProcessStack[PROCESS_STACK_SIZE];
static char TerminalProcessStack[PROCESS_STACK_SIZE];
static char PAstack[PROCESS_STACK_SIZE];
static char PBstack[PROCESS_STACK_SIZE];

void NullProcess (void)
{
    for (;;) {
        KernelRelinquish();	/* null process simply surrenders processor */
    }
}

/* relinquish then check for jtag input. Can have commands like display processes and state
 * (save states in kernel and then read from those to display) and commands for sending a message */

void TerminalProcess (void)
{
    char input[30];
    for (;;) {
        scanf("%s", input);
        printf("%s\n", input);

        if (strcmp(input, "processA") == 0) {
            QuenosNewProcess(ProcessA, PAstack, PROCESS_STACK_SIZE);
            KernelRelinquish();
        } else if (strcmp(input, "processB") == 0) {
            QuenosNewProcess(ProcessB, PBstack, PROCESS_STACK_SIZE);
            KernelRelinquish();
        }
    }
}

int main ()
{
    /* initialize interrupt vectors and any other things... */
    QuenosInit();

    /* create null process and add to ready queue */
    QuenosNewProcess(NullProcess, NullProcessStack,
                     PROCESS_STACK_SIZE);

    // Create terminal process
    QuenosNewProcess(TerminalProcess, TerminalProcessStack,
                     PROCESS_STACK_SIZE);

    /* create user processes and add to ready queue */
    /* UserProcesses (); */

    /* start up the first process (we never return here) */
    QuenosDispatch();
}

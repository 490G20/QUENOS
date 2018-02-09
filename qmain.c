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

#define PROCESS_STACK_SIZE 256 // TODO: confirm is same for altera nios 2, if not update

static char NullProcessStack[PROCESS_STACK_SIZE];
static char TerminalProcessStack[PROCESS_STACK_SIZE];

volatile int* JTAG_UART_ptr;

void NullProcess (void)
{
  for (;;) {
    KernelRelinquish ();	/* null process simply surrenders processor */
  }
}

/* relinquish then check for jtag input. Can have commands like display processes and state
 * (save states in kernel and then read from those to display) and commands for sending a message */

char get_char( void )
{
    int data;
    data = *(JTAG_UART_ptr); // read the JTAG_UART data register
    if (data & 0x00008000) // check RVALID to see if there is new data
        return ((char) data & 0xFF);
    else
        return '\0';
}

void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

void TerminalProcess (void)
{
    char input[30];
    for (;;) {
        char i;
        i = get_char();

        while (i != '\0') {
            put_jtag(JTAG_UART_ptr,i);

            if (i == '\n') {
                if (strcmp(input, "queue") == 0) {
                    ShowReadyQueue();
                } else if (strcmp(input, "ab") == 0) {
                    printString("hey\n");
                }

                memset(input, 0, sizeof(input));
                break;
            }

            append(input, i);
            i = get_char();
        }

        KernelRelinquish();
    }
}

int main ()
{
  /* initialize interrupt vectors and any other things... */
  QuenosInit ();

  /* create null process and add to ready queue */
  QuenosNewProcess (NullProcess, NullProcessStack, PROCESS_STACK_SIZE);

  /* create user processes and add to ready queue */
  UserProcesses ();

  QuenosNewProcess (TerminalProcess, TerminalProcessStack, PROCESS_STACK_SIZE);

  /* start up the first process (we never return here) */
  QuenosDispatch ();
}

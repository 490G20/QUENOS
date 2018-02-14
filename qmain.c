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
#include <ctype.h>

#define PROCESS_STACK_SIZE 256 // TODO: confirm is same for altera nios 2, if not update

static char NullProcessStack[PROCESS_STACK_SIZE];
static char TerminalProcessStack[PROCESS_STACK_SIZE];

volatile int* JTAG_UART = (int*) 0x10001000;// JTAG UART address

/* extern Process *process_array_p[MAX_NUM_OF_PROCESSES]; */
/* char processQueues[7][32]; */

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
    data = *(JTAG_UART); // read the JTAG_UART data register
    if (data & 0x00008000) // check RVALID to see if there is new data
        return ((char) data & 0xFF);
    else
        return ('\0');
}

void append(char* s, char c) {
        int len = strlen(s);
        s[len] = c;
        s[len+1] = '\0';
}

int startsWith(const char *pre, const char *str) {
    int lenpre = strlen(pre);
    int lenstr = strlen(str);
    return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

void TerminalProcess (void)
{
    char input[30];
    printString("> ");
    for (;;) {
        char i;
        i = get_char();

        while (i != '\0') {
            put_jtag(i);

            if (i == '\n') {
                if (strcmp(input, "queue") == 0) {
                    ShowReadyQueue();
                } else if (strcmp(input, "hi") == 0) {
                    printString("hey\n");
                } else if (startsWith("send ", input)) {
                    Message m;
                    int j = 5;
                    int k = 0;
                    while (input[j] != '\0')
                    {
                        m.data[k] = input[j];
                        j++;
                        k++;
                    }
                    m.data[k] = '\0';
                    m.next = 0;
                    m.prev = 0;
                    KernelSendMessage(5, &m);
                /* } else if (startsWith("queue ", input)) { */
                /*     if (isdigit(input[6])) */
                /*     { */
                /*         int pid = input[6] - '0'; */
                /*         printString("queue for process "); */
                /*         put_jtag(JTAG_UART_ptr, '0'+pid); */
                /*         printString(": "); */
                /*         printString(processQueues[pid]); */
                /*         printString("\n"); */
                /*     } else { */
                /*         printString("Please enter a digit for the queue pid\n"); */
                /*     } */
                /* } else if (startsWith("state ", input)) { */
                /*     if (isdigit(input[6])) */
                /*     { */
                /*         int pid = input[6] - '0'; */
                /*         printString("state for process "); */
                /*         put_jtag('0'+pid); */
                /*         printString(": "); */
                /*         if (process_array_p[pid]->state == READY) */
                /*                 printString(" ready\n"); */
                /*         else if (process_array_p[pid]->state == BLOCKED) */
                /*                 printString(" blocked\n"); */
                /*         else if (process_array_p[pid]->state == RUNNING) */
                /*                 printString(" running\n"); */
                /*         else if (process_array_p[pid]->state == WAITING_FOR_MESSAGE) */
                /*                 printString(" waiting for message\n"); */
                /*     } else { */
                /*         printString("Please enter a digit for the process pid\n"); */
                /*     } */
                }

                memset(input, 0, sizeof(input));
                printString("> ");
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

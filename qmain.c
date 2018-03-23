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

extern volatile unsigned int* JTAG_UART_ptr;

void put_jtag(volatile unsigned int* JTAG_UART_ptr, unsigned int c);

void NullProcess (void)
{
  for (;;) {
    printString("0\t");
    KernelRelinquish ();	/* null process simply surrenders processor */
  }
}

/* relinquish then check for jtag input. Can have commands like display processes and state
 * (save states in kernel and then read from those to display) and commands for sending a message */

char get_char( void )
{
    unsigned int data;
    data = *(JTAG_UART_ptr); // read the JTAG_UART data register
    if (data & 0x00008000) // check RVALID to see if there is new data
        return (data & 0xFF);
    else
        return '\0';
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
    input[0] = '\0';
    for (;;) {
        char i;
        i = get_char();

        while (i != '\0' && i < 128 && i > 0) {
            put_jtag(JTAG_UART_ptr,i);

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
                }

                input[0] = '\0';
                printString("> ");
                break;
            }

            if (i < 128 && i > 0){
                append(input, i);
            }
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

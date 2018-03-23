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

#define PROCESS_STACK_SIZE 256

static char NullProcessStack[PROCESS_STACK_SIZE];
static char TerminalProcessStack[PROCESS_STACK_SIZE];

extern volatile unsigned int* JTAG_UART_ptr;

void put_jtag(volatile unsigned int* JTAG_UART_ptr, unsigned int c);

// Null process simply surrenders processor
void NullProcess(void)
{
  for (;;) {
    KernelRelinquish();
  }
}

// Get a character from JTAG UART input
char get_char(void)
{
  // read the JTAG_UART data register
  unsigned int data;
  data = *(JTAG_UART_ptr);

  // check RVALID to see if there is new data
  if (data & 0x00008000)
    return (data & 0xFF);
  else
    // Returns empty character if there is no new character
    return '\0';
}

// Appends a character to an array of chars
void append(char* s, char c) {
  int len = strlen(s);
  s[len] = c;
  s[len+1] = '\0';
}

// Function that returns 1 if the str parameter starts with the pre parameter
int startsWith(const char *pre, const char *str) {
  int lenpre = strlen(pre);
  int lenstr = strlen(str);
  return lenstr < lenpre ? 0 : strncmp(pre, str, lenpre) == 0;
}

// Terminal process that takes in input from JTAG UART, but constantly relinquishes itself
// if there is no new character available in order to not block the kernel. There is currently
// only three commands: queue, hi and send. Queue will show the current ready queue. Hi will
// simply print 'hey'. Send will send the message given as the argument to process 5 that is
// blocked waiting for a message.
void TerminalProcess(void)
{
  char input[30];

  // Prints '>' in order to replicate a command line accepting input
  printString("> ");

  // Initializes the input char array to an empty character
  input[0] = '\0';

  for (;;) {
    // Gets a character from JTAG UART input
    char i;
    i = get_char();

    // While the new character grabbed is not an empty or non-valid character
    while (i != '\0' && i < 128 && i > 0) {
      // Prints the character
      put_jtag(JTAG_UART_ptr,i);

      // If the enter key is given, process the input character array. Newline
      // means the user has finished typing in the command.
      if (i == '\n') {
        if (strcmp(input, "queue") == 0) {
          // Shows the current ready queue
          ShowReadyQueue();
        } else if (strcmp(input, "hi") == 0) {
          printString("hey\n");
        } else if (startsWith("send ", input)) {
          Message m;
          int j = 5; // The message that is to be sent starts at the 5th character in the command string
          int k = 0;

          while (input[j] != '\0')
          {
            // Saves the message into the Message data
            m.data[k] = input[j];
            j++;
            k++;
          }

          // Appends the last character of the message data to be an empty character
          m.data[k] = '\0';
          m.next = 0;
          m.prev = 0;

          // Sends the message
          KernelSendMessage(5, &m);
        }

        // Resets the input character array to an empty character
        input[0] = '\0';
        printString("> ");
        break;
      }

      // If the character is valid, append it to the input string
      if (i < 128 && i > 0)
      {
        append(input, i);
      }

      // Gets a new character
      i = get_char();
    }

    KernelRelinquish();
  }
}

int main ()
{
  // Initialize interrupt vectors and any other things...
  QuenosInit();

  // Create null process and add to ready queue
  QuenosNewProcess(NullProcess, NullProcessStack, PROCESS_STACK_SIZE);

  // Create user processes and add to ready queue
  UserProcesses();

  // Create terminal process and add to ready queue
  QuenosNewProcess(TerminalProcess, TerminalProcessStack, PROCESS_STACK_SIZE);

  // Start up the first process (we never return here)
  QuenosDispatch();
}

/******************************************************************************
NAME:		quser.c
DESCRIPTION:	Definitions of functions for user processes, including a
		function to create them all on startup. Here, two processes
		are created. The first simply blocks itself. The second
		unblocks itself, and then relinquishes the processor. With
		the first process created before the second process, it will
		be executed first. Hence, it will always block itself just
		before the second process unblocks it.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "quser.h"

#define USER_STACK_SIZE 256

static char P1stack[USER_STACK_SIZE];
static char P2stack[USER_STACK_SIZE];
static char P3stack[USER_STACK_SIZE];
static char P4stack[USER_STACK_SIZE];
static char P5stack[USER_STACK_SIZE];
static char P6stack[USER_STACK_SIZE];
static char P7stack[USER_STACK_SIZE];

volatile int* JTAG_UART_ptr; // JTAG UART address

void short_delay(volatile unsigned long count)
{
  asm("					;\
          ldw r4, 0(sp)		;\
  loop:					;\
          subi r4, r4, 1		;\
          bne r4, r0, loop	;\
  ");
}

// Process 1 simply blocks itself
static void Process1(void)
{
  for (;;)
  {
    short_delay(5);
    KernelBlock();
  }
}

// Process 2 unblocks process 1 and then relinquishes itself
static void Process2(void)
{
  for (;;)
  {
    /* short_delay(5); */
    KernelUnblock(1);
    KernelRelinquish();
  }
}

// Process 3 prints the message given from process 4
static void Process3(void)
{
  for (;;)
  {
    short_delay(5);
    Message *m;
    m = KernelReadMessage();

    // If the message is not empty, print the message
    if (m != 0)
    {
      int i;
      for (i=0; i < strlen(m->data); i++) {
        // The print is commented out so that it will not spam the message to the terminal
        /* put_jtag(JTAG_UART_ptr, m->data[i]); */
      }
      /* printString("\n"); */
    }
    else
    {
      printString("No message!");
    }

    KernelRelinquish();
  }
}

// Process 4 sends a message to process 3
static void Process4(void)
{
  Message m;
  m.data[0] = 'h';
  m.data[1] = 'e';
  m.data[2] = 'l';
  m.data[3] = 'l';
  m.data[4] = 'o';
  m.data[5] = '\0';
  m.next = 0;
  m.prev = 0;

  for (;;)
  {
    short_delay(5);
    KernelSendMessage(3, &m);
    KernelRelinquish();
  }
}

// Process 5 waits for a pushbutton interrupt
static void Process5(void)
{
  for (;;)
  {
    short_delay(5);
    KernelPBBlock();
  }
}

// Process 6 waits for a message from the terminal process
static void Process6(void)
{
  for (;;)
  {
    Message *m;
    m = KernelReadMessage();
    printString("Process 5 recieved this message: ");
    int i;
    for (i=0; i < strlen(m->data); i++) {
      put_jtag(JTAG_UART_ptr, m->data[i]);
    }
    printString("\n");

    KernelRelinquish();
  }
}

// Process 7 waits for a timer interrupt
static void Process7(void) {
  for (;;)
  {
    KernelTimerDelay(10000);
  }
}


// Called from main program at startup
void UserProcesses(void)
{
  QuenosNewProcess(Process1, P1stack, USER_STACK_SIZE);
  QuenosNewProcess(Process2, P2stack, USER_STACK_SIZE);
  QuenosNewProcess(Process3, P3stack, USER_STACK_SIZE);
  QuenosNewProcess(Process4, P4stack, USER_STACK_SIZE);
  QuenosNewProcess(Process5, P5stack, USER_STACK_SIZE);
  QuenosNewProcess(Process6, P6stack, USER_STACK_SIZE);
  QuenosNewProcess(Process7, P7stack, USER_STACK_SIZE);
}

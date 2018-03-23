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
#include "qcore.h"

#define USER_STACK_SIZE 256

static  char    TimerProcessStack[USER_STACK_SIZE];
static char P1stack[USER_STACK_SIZE];
static char P2stack[USER_STACK_SIZE];
static char P3stack[USER_STACK_SIZE];
static char P4stack[USER_STACK_SIZE];
static char P5stack[USER_STACK_SIZE];

volatile int* JTAG_UART_ptr; // JTAG UART address
/**
 * Application binary interface documentation says other_pid will be passed into r4
 */

 void short_delay (volatile unsigned long count)
{
	asm("					;\
		ldw r4, 0(sp)		;\
	loop:					;\
		subi r4, r4, 1		;\
		bne r4, r0, loop	;\
	");
}

 static void TimerProcess (void) {
	for (;;)
	{
		printString("1\t");
		KernelTimerDelay(10000);
	}
 }
 
static void Process1 (void)
{
    for (;;)
    {
	short_delay(42000000);
        printString("2\t");
        KernelBlock();
    }
}

static void Process2 (void)
{
    for (;;)
    {
	short_delay(42000000);
        printString("3\t");
        KernelUnblock(1);
        KernelRelinquish();
    }
}
/*
static void Process3 (void)
{
        for (;;)
        {
            /* printString("3\n"); */
            Message *m;
            m = KernelReadMessage();
            if (m != 0){
                int i;
                for (i=0; i < strlen(m->data); i++) {
                    /* put_jtag(JTAG_UART_ptr, m->data[i]); */
                }
                /* printString("\n"); */
            }
            else {
                /* printString("no message"); */
            }

            KernelRelinquish();
        }
}

static void Process4 (void)
{
    Message m;
    m.data[0] = 'h'; // char array
    m.data[1] = 'e'; // char array
    m.data[2] = 'l'; // char array
    m.data[3] = 'l'; // char array
    m.data[4] = 'o'; // char array
    m.data[5] = '\0'; // char array
    m.next = 0;
    m.prev = 0;
    for (;;)
    {
        /* printString("4\n"); */
        KernelSendMessage(3, &m);
        KernelRelinquish();
    }
}
*/
static void Process5 (void)
{
	for (;;)
	{	
		short_delay(42000000);
    printString("4\t");
		KernelPBBlock();
		printString("Process_Pressed!\n");
	}
}

static void Process5 (void)
{
        for (;;)
        {
            /* printString("5\n"); */
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

void UserProcesses (void)
{
    QuenosNewProcess (TimerProcess, TimerProcessStack, USER_STACK_SIZE);
    QuenosNewProcess (Process1, P1stack, USER_STACK_SIZE);
    QuenosNewProcess (Process2, P2stack, USER_STACK_SIZE);
    QuenosNewProcess (Process3, P3stack, USER_STACK_SIZE );
    QuenosNewProcess (Process4, P4stack, USER_STACK_SIZE );
    QuenosNewProcess (Process5, P5stack, USER_STACK_SIZE );
    QuenosNewProcess (Process5, P5stack, USER_STACK_SIZE);
}

/*----------------------------------------------------------------*/
/* called from main program on startup */



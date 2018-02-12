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
//static  char    P1stack[USER_STACK_SIZE];
//static  char    P2stack[USER_STACK_SIZE];
//static  char    P3stack[USER_STACK_SIZE];
//static  char    P4stack[USER_STACK_SIZE];

volatile int* JTAG_UART_ptr; // JTAG UART address
/**
 * Application binary interface documentation says other_pid will be passed into r4
 */

 static void TimerProcess (void) {
	for (;;)
	{
		printString("Timer\n");
		KernelTimerDelay(1000);
	}
 }
 
//static void Process1 (void)
//{
//    for (;;)
//    {
//        printString("1\n");
//        KernelBlock();
//    }
//}
//
//static void Process2 (void)
//{
//    for (;;)
//    {
//        printString("2\n");
//        KernelUnblock(1);
//        KernelRelinquish();
//    }
//}
//
//static void Process3 (void)
//{
//        for (;;)
//        {
//            printString("3\n");
//            Message *m;
//            m = KernelReadMessage();
//            if (m != 0){
//                int i;
//                for (i=0; i < strlen(m->data); i++) {
//                    put_jtag(JTAG_UART_ptr, m->data[i]);
//                }
//                printString("\n");
//            }
//            else {
//                printString("no message");
//            }
//
//            KernelRelinquish();
//        }
//}
//
//static void Process4 (void)
//{
//    Message m;
//    m.data[0] = 'h'; // char array
//    m.data[1] = 'e'; // char array
//    m.data[2] = 'l'; // char array
//    m.data[3] = 'l'; // char array
//    m.data[4] = 'o'; // char array
//    m.data[5] = '\0'; // char array
//    m.next = 0;
//    m.prev = 0;
//    for (;;)
//    {
//        printString("4\n");
//        KernelSendMessage(3, &m);
//        KernelRelinquish();
//    }
//}

void UserProcesses (void)
{
    QuenosNewProcess (TimerProcess, TimerProcessStack, USER_STACK_SIZE);
//    QuenosNewProcess (Process1, P1stack, USER_STACK_SIZE);
//    QuenosNewProcess (Process2, P2stack, USER_STACK_SIZE);
//    QuenosNewProcess (Process3, P3stack, USER_STACK_SIZE );
//    QuenosNewProcess (Process4, P4stack, USER_STACK_SIZE );
}

/*----------------------------------------------------------------*/
/* called from main program on startup */



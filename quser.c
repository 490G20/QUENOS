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
static  char    P0stack[USER_STACK_SIZE];
static  char    P1stack[USER_STACK_SIZE];
static  char    P2stack[USER_STACK_SIZE];

volatile int* JTAG_UART_ptr; // JTAG UART address
/**
 * Application binary interface documentation says other_pid will be passed into r4
 */
void    QuenosUnblock (int other_pid) {
    KernelUnblock(other_pid);
}
//yolo swag
static void Process0(void){
    for (;;){
        KernelRelinquish();
    }
}

/**
target_pid in r4, message address into r5
*/
void QuenosSendMessage(int target_pid, Message *messageAddress){
	KernelSendMessage(target_pid, messageAddress);
//	if (process_array[target_pid].state == BLOCKED){
//		/* only unblock and add to ready queue if it was blocked */
//        process_array[target_pid].state = READY;
//        AddToTail (&ready_queue, &process_array[target_pid]);
//    }
//
//	KernelRelinquish (); //relinquish elsewhere?
}

unsigned int QuenosReceiveMessage(){
    return KernelReadMessage();
}

static	void    Process1 (void)
{
        for (;;)
        {
            char m;
            printString("a\n");
            //KernelBlock();
            m = QuenosReceiveMessage();
            if (m != 0){
                put_jtag(JTAG_UART_ptr, m);
            }
            else {
                printString("no message");
            }

            KernelRelinquish();
        }
}

static	void    Process2 (void)
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
        printString("b\n");
        //QuenosUnblock (1);
        QuenosSendMessage(1, &m);
        KernelRelinquish();
    }
}

void UserProcesses (void)
{
    QuenosNewProcess(Process0, P0stack, USER_STACK_SIZE );
    QuenosNewProcess (Process1, P1stack, USER_STACK_SIZE);
    QuenosNewProcess (Process2, P2stack, USER_STACK_SIZE);
}

/*----------------------------------------------------------------*/
/* called from main program on startup */



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

static  char    P1stack[USER_STACK_SIZE];
static  char    P2stack[USER_STACK_SIZE];

/**
 * Application binary interface documentation says other_pid will be passed into r4
 */
void    QuenosUnblock (int other_pid) {
    KernelUnblock();
}

/**
target_pid in r4, message address into r5
*/
void QuenosSendMessage(int target_pid, unsigned int messageAddress){//Message *m){

    // Shove message (address?) somewhere kernel can get at it



	KernelSendMessage();
	
//	if (process_array[target_pid].state == BLOCKED){
//		/* only unblock and add to ready queue if it was blocked */
//        process_array[target_pid].state = READY;
//        AddToTail (&ready_queue, &process_array[target_pid]);
//    }
//
//	KernelRelinquish (); //relinquish elsewhere?
}

void QuenosReceiveMessage(){

}

static	void    Process1 (void)
{	
        for (;;)
        {
                printString("a\n");
                //KernelBlock();
                KernelReadMessage();

        }
		
		
}

static	void    Process2 (void)
{
        for (;;)
        {
                printString("b\n");
				QuenosUnblock (1);
				KernelRelinquish ();
        }
}

void UserProcesses (void)
{
    QuenosNewProcess (Process1, P1stack, USER_STACK_SIZE);
    QuenosNewProcess (Process2, P2stack, USER_STACK_SIZE);
}

/*----------------------------------------------------------------*/
/* called from main program on startup */



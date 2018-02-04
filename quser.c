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

static char P1stack[USER_STACK_SIZE];
static char P2stack[USER_STACK_SIZE];
volatile int *red_LED_ptr = (int *) 0x10000000; // red LED address
volatile int *green_LED_ptr = (int *) 0x10000010; // green LED address

/**
 * Application binary interface documentation says other_pid will be passed into r4
 */
void QuenosUnblock (int other_pid) {
    KernelUnblock();
}

static void Process1 (void)
{
    for (;;)
    {
        printString("a\n");
        KernelBlock();
    }
}

static void Process2 (void)
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

void ProcessA (void)
{
    *(green_LED_ptr) = 1; // light up the green LEDs
    KernelBlock();
    *(red_LED_ptr) = 1; // light up the green LEDs
    *(green_LED_ptr) = 0; // light up the green LEDs
}

void ProcessB (void)
{
    *(green_LED_ptr) = 3; // light up the green LEDs
    QuenosUnblock (2);
    *(red_LED_ptr) = 1; // light up the green LEDs
    *(green_LED_ptr) = 1; // light up the green LEDs
}

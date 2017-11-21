/******************************************************************************
NAME:		qrequest.c
DESCRIPTION:	Definitions for functions to request specific kernel services.
		In-line assembly code is used to produce the software interrupt
		that is necessary to enter the kernel. The type of request is
		passed to the kernel (i.e., pushed on the stack with the
		interrupt) in accumulator A. If another parameter is needed
		(as in the case of blocking), it is passed in accumulator B.
		Because accumulator A is the high 8 bits of accumulator D,
		the request code is shifted left by 8 bits. The 'addd #0' is
		needed only because without it, there is no way of passing
		the request type to the in-line assembly code. The compiler
		generates code before the code in quotations below to set
		the contents of accumulator D, but only if the code in
		quotations has an instruction like 'addd #0' that actually
		uses accumulator D.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
//#include <memory.h>
#include <string.h>
#include "qrequest.h"

void    QuenosRelinquish (void)
{
    asm("subi sp, sp, 4");
    asm("add r23,r23,r0");
    asm("addi r23,1"); //Relinquish enum
    asm("stw r23, 4(sp)");
    asm("trap");
}

void    QuenosBlockSelf (void)
{
    asm("subi sp, sp, 4");
    asm("add r23,r23,r0");
    asm("addi r23,2"); // #block self enum
    asm("stw r23, 4(sp)");
    asm("trap");
}

/**
 *
 * We will see if we will need to use the specific register that is the 1st argument in a C
 * call to assembly instead of arbitrarily using our choice of r22
 */

void    QuenosUnblock (int other_pid)
{

    asm("subi sp, sp, 8");

    // Convert other_pid into a string and concat it in

    int length = snprintf(NULL, 0, "%d", other_pid);
    char* str = malloc (length +1); //1 extra character for null terminator
    //TODO: Check errors in malloc
    snprintf(str, length+1, "%d", other_pid);

    char assembly_command[] = "movi r22,";

    char *completed_command = malloc(strlen(assembly_command) + strlen(str) +1);
    //TODO: Check for errors in malloc
    strcpy(completed_command, assembly_command);
    strcat(completed_command, str);

    _asm(completed_command); //String literal and other problems, perhaps we must do as originally suggested with the full assembly, and compiler register convention tricks

    asm("add r23,r23,r0");
    asm("addi r23,3"); //unblock enum
    asm("stw r23, 4(sp)");
    asm("stw r22, 8(sp)");
    asm("trap");

    free(str);
    free(completed_command);
}


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
#include "nios2_ctrl_reg_macros.h"

#define NULL_PROCESS_STACK_SIZE 256 // TODO: confirm is same for altera nios 2, if not update

volatile int* JTAG_UART_ptr; // JTAG UART address
volatile int* interval_timer_ptr; // JTAG UART address

static char NullProcessStack[NULL_PROCESS_STACK_SIZE];

void NullProcess (void)
{
  for (;;) {
    int status = *(interval_timer_ptr + 0x4)+ (*(interval_timer_ptr + 0x5) << 16);
    //printString(s);
    int i = 0;
    int j = 0;
    for (i = 0; i< 8; i++) {
      int power = 1;
      for (j = 7; j > i; j--){
         power = power * 10;
      }
      put_jtag(JTAG_UART_ptr,'0'+(status/power)%10);
    }
    asm("mov r3, r0");
    asm("addi r3,r3,0x1");
    asm("movi r4, 0xFFFF");
    asm("bne r3, r4, 0xC28");
    
    //printf("0x%d\n",status);
    printString("\nNull\n");
    KernelRelinquish ();	/* null process simply surrenders processor */
  }
}

int main ()
{
  /* initialize interrupt vectors and any other things... */
  QuenosInit ();

  /* create null process and add to ready queue */
  QuenosNewProcess(NullProcess, NullProcessStack, NULL_PROCESS_STACK_SIZE);

  /* create user processes and add to ready queue */
  UserProcesses ();

  /* start up the first process (we never return here) */
  QuenosDispatch ();
}

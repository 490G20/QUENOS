/******************************************************************************
NAME:		qinit.c
DESCRIPTION:	Definition of function for initialization on startup.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "nios2_ctrl_reg_macros.h"
extern volatile int * interval_timer_ptr;
extern int timerInterval;

void QuenosInit (void)
{
  /* set the software interrupt vector to enter the kernel */

  /**
  0000 0000 0000 0000 0000 0111 0000 0001 = 0x0701
  Bit 0: Interval timer
  Bit 10: Serial port
  Bit 8/9(?): JTAG port
  **/
	int counter = msec * 50 * 1000000; //0x190000; // 1/(50 MHz) × (0x190000) = 33 msec
	*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
	*(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
	*(interval_timer_ptr + 1) = 0x7; 		// STOP = 0, START = 1, CONT = 1, ITO = 1

  NIOS2_WRITE_IENABLE(0x0703);
  NIOS2_WRITE_STATUS(1);
}

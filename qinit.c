/******************************************************************************
NAME:		qinit.c
DESCRIPTION:	Definition of function for initialization on startup.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "nios2_ctrl_reg_macros.h"

volatile int *KEY_ptr = (int *) 0x10000050; // pushbutton KEY address

void QuenosInit (void)
{
  /* set the software interrupt vector to enter the kernel */

  *(KEY_ptr + 2) = 0xE; /* write to the pushbutton interrupt mask register, and
                        * set 3 mask bits to 1 (bit 0 is Nios II reset) */
  /**
  0000 0000 0000 0000 0000 0111 0000 0011 = 0x0703
  Bit 0: Interval timer
  Bit 10: Serial port
  Bit 8/9(?): JTAG port
  **/
  NIOS2_WRITE_IENABLE(0x0703);
  NIOS2_WRITE_STATUS(1);
}

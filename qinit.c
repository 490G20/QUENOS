/******************************************************************************
NAME:		qinit.c
DESCRIPTION:	Definition of function for initialization on startup.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "qcore.h"
#include "DBug12.h"
#include "nios2_ctrl_reg_macros.h"

void    QuenosInit (void)
{
	/* set the software interrupt vector to enter the kernel */

	/**
	0000 0000 0000 0000 0000 0111 0000 0001 = 0x0701
	Bit 0: Interval timer
	Bit 10: Serial port
	Bit 8/9(?): JTAG port
	**/
	NIOS2_WRITE_IENABLE(0x0701);
	NIOS2_WRITE_STATUS(1);
}

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

void    QuerkInit (void)
{
	/* set the software interrupt vector to enter the kernel */        
        //SetUserVector (UserSWI, (void *) QuerkSWIHandler);
	//SetUserVector is for motorola software interrupt initialization, and 
	//takes the SWI name and the method that handles it as input

	volatile int* interval_timer_ptr = (int*)0x10002000;
	// interval timer base address
	volatile int* KEY_ptr = (int*) 0x10000050;// pushbutton KEY address
	/* set the interval timer period for scrolling the HEX displays */
	int counter = 0x190000;// 1/(50 MHz)£(0x190000) = 33 msec
	*(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
	*(interval_timer_ptr + 0x3) = (counter»»16) & 0xFFFF;

	/* start interval timer, enable its interrupts */
	*(interval_timer_ptr + 1) = 0x7; // STOP = 0, START = 1, CONT = 1, ITO = 1	
	//TODO: Enable JTAG UART interrupts



	/**
	0000 0000 0000 0000 0000 0111 0000 0001 = 0x0701
	Bit 0: Interval timer
	Bit 10: Serial port
	Bit 8/9(?): JTAG port
	**/
	NIOS2_WRITE_IENABLE(0x0701); 
	NIOS2_WRITE_STATUS(1);
		
}

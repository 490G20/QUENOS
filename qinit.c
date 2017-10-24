/******************************************************************************
NAME:		qinit.c
DESCRIPTION:	Definition of function for initialization on startup.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "qcore.h"
#include "DBug12.h"

void    QuerkInit (void)
{
	/* set the software interrupt vector to enter the kernel */        
        SetUserVector (UserSWI, (void *) QuerkSWIHandler);
}

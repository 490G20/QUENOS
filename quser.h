#ifndef _QUSER_H_
#define _QUSER_H_

/******************************************************************************
NAME:		quser.h
DESCRIPTION:	Declarations for user-supplied function to create
		user processes.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

extern  void    UserProcesses (void);
// Below code is copied from qrequest.h
// This is done as an attempt to remove the qrequest.c/.h files from the program as we believe they are redundant and may be messing up our stack pointer 
typedef enum    {Relinquish, BlockSelf, Unblock} Request;

extern	void    QuenosUnblock (int other_pid);

extern void KernelRelinquish(void);
extern void KernelBlock(void);
extern void KernelUnblock(void);

#endif /* _QUSER_H_ */

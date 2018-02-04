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
typedef enum    {Relinquish, BlockSelf, Unblock} Request;

extern	void    QuenosUnblock (int other_pid);

extern void KernelRelinquish(void);
extern void KernelBlock(void);
extern void KernelUnblock(void);

extern void ProcessA(void);
extern void ProcessB(void);
#endif /* _QUSER_H_ */

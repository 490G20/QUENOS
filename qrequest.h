#ifndef _QREQUEST_H_
#define _QREQUEST_H_

/******************************************************************************
NAME:		qrequest.h
DESCRIPTION:	Declarations for functions to request specific kernel services.
		Also defines enumerated type for request codes.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

typedef enum    {Relinquish, BlockSelf, Unblock} Request; //TODO: investigate if this is smart enough to auto equal 1,2,3

extern	void    QuenosRelinquish (void);
extern	void    QuenosBlockSelf (void);
extern	void    QuenosUnblock (int other_pid);

extern void KernelRelinquish(void);
extern void KernelBlock(void);
extern void KernelUnblock(void);

#endif //_QREQUEST_H_

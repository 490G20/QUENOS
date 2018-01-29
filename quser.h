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
typedef enum    {RELINQUISH, BLOCK_SELF, UNBLOCK, SEND_MESSAGE, READ_MESSAGE} Request;

extern	void    QuenosUnblock (int other_pid);

extern void KernelRelinquish(void);
extern void KernelBlock(void);
extern void KernelUnblock(void);
extern void KernelSendMessage(void);
extern void *KernelReadMessage(void);

/**
 * We note that KernelUnblock and KernelSendMessage have no arguments here because according to application binary
 * interface documentation that we will have the C code pass in C method's parameters into registers r4, r5,
 */

#endif /* _QUSER_H_ */

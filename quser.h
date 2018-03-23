#ifndef _QUSER_H_
#define _QUSER_H_

#include "qcore.h"

/******************************************************************************
NAME:		quser.h
DESCRIPTION:	Declarations for user-supplied function to create
		user processes.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

extern void UserProcesses(void);
typedef enum {RELINQUISH, BLOCK_SELF, UNBLOCK, SEND_MESSAGE, READ_MESSAGE, TIMER_DELAY, PB_BLOCK} Request;

extern void KernelRelinquish(void);
extern void KernelBlock(void);
extern void KernelUnblock(int other_pid);
extern void KernelTimerDelay(unsigned int delaytime);
extern void KernelSendMessage(int target_pid, Message *messageAddress);
extern void *KernelReadMessage(void);
extern void KernelPBBlock(void);

#endif /* _QUSER_H_ */

#ifndef _QUEUE_H_
#define _QUEUE_H_

/******************************************************************************
NAME:		queue.h
DESCRIPTION:	Declarations for process descriptor queue management functions.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/
#include "qcore.h"

typedef struct  _queue
{
        Process     *head;
        Process     *tail;
} Queue;

extern  void    AddToTail (Queue *queue, Process *process);
extern  Process     *DequeueHead (Queue *queue);

// MessageQueue is essentially identical to the (process) queue, except it is for Message objects (structs)
typedef struct _messageQueue { // watch the capital
    Message *head;
    Message *tail;
} MessageQueue;

extern void AddMessageToTail(MessageQueue *q, Message *m);
extern Message *DequeueMessageHead(DequeueMessageHead *queue);

#endif /* _QUEUE_H_ */


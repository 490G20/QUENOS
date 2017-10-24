#ifndef _QUEUE_H_
#define _QUEUE_H_

/******************************************************************************
NAME:		queue.h
DESCRIPTION:	Declarations for process descriptor queue management functions.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

typedef struct  _queue
{
        PDB     *head;
        PDB     *tail;
} Queue;

extern  void    AddToTail (Queue *queue, PDB *pdb);
extern  PDB     *DequeueHead (Queue *queue);

#endif /* _QUEUE_H_ */


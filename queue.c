/******************************************************************************
NAME:		queue.c
DESCRIPTION:	Definitions of process descriptor queue management functions.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "qcore.h"
#include "queue.h"

/*----------------------------------------------------------------*/

void    AddToTail (Queue *queue, PDB *pdb)
{
        pdb->prev = queue->tail;
        pdb->next = 0;
        if (queue->head != 0)
                queue->tail->next = pdb;
        else
                queue->head = pdb;
        queue->tail = pdb;
}

PDB     *DequeueHead (Queue *queue)
{
        PDB     *ret;

        ret = queue->head;
        if (ret == 0)
                return ret;

        queue->head = ret->next;
        if (queue->head == 0)
                queue->tail = 0;
        else
                queue->head->prev = 0;

        return ret;
}

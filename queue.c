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

void    AddToTail (Queue *queue, Process *process)
{
        process->prev = queue->tail;
        process->next = 0;
        if (queue->head != 0)
            queue->tail->next = process;// not empty add to end
        else
            queue->head = process;//queue was empty, set new thing to head
        queue->tail = process;
}

Process     *DequeueHead (Queue *queue)
{
        Process     *ret;

        ret = queue->head;
        if (ret == 0)
            return ret; //queue is empty return null (0)

        queue->head = ret->next;
        if (queue->head == 0)
            queue->tail = 0; //queue is now empty
        else
            queue->head->prev = 0;// queue is not empty

        return ret;
}

// C doesn't have strong OOP support so this code is pretty heavily rehashed from the original queue for processes

void    AddMessageToTail (MessageQueue *queue, Message *message)
{
    message->prev = queue->tail;
    message->next = 0;
    if (queue->head != 0)
        queue->tail->next = message;
    else
        queue->head = message;
    queue->tail = message;
}

Message     *DequeueMessageHead (MessageQueue *queue)
{
    Message     *ret;

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

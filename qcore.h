#ifndef _QCORE_H_
#define _QCORE_H_

/******************************************************************************
NAME:		qcore.h
DESCRIPTION:	Type definitions and function declarations for core of
		QUERK kernel.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

typedef enum {READY, RUNNING, BLOCKED, WAITING_FOR_MESSAGE, DELAYED} State;

typedef struct _message {
	struct _message *prev;
	struct _message *next;
	unsigned char data[32]; // Revise into hardcoded, and do dynamic mem allocation later if necessary for proof of concept
} Message;

// MessageQueue is essentially identical to the (process) queue, except it is for Message objects (structs)
typedef struct _messageQueue { // watch the capital?
    Message *head;
    Message *tail;
} MessageQueue;

extern void AddMessageToTail(MessageQueue *queue, Message *message);
extern Message *DequeueMessageHead (MessageQueue *queue);

typedef struct  _process // Formerly _pdb
{
        unsigned int program_address;
        struct  _process    *prev;
        struct  _process    *next;
        int     pid; // process id
        State   state;
        unsigned int interrupt_delay;
		// TODO: Best to name user stack pointer or stack pointer? formerly just SP
        void    *user_stack_pointer;		/* saves user stack pointer when not running */

        struct _messageQueue m_queue; // Move elsewhere if debugging with hairy pointer math
} Process; // Formerly pdb

#define MAX_NUM_OF_PROCESSES 16

extern  void    QuenosNewProcess (void (*entry_point) (void),
				 char *stack_bottom, int stack_size);

extern  void    QuenosSWIHandler (void);

extern  void    QuenosDispatch (void);
extern unsigned int process_stack_pointer;
extern unsigned int ksp;

#endif /* _QCORE_H_ */

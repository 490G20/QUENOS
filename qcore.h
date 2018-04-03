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

#define MAX_NUM_OF_PROCESSES 16

typedef enum {READY, RUNNING, BLOCKED, WAITING_FOR_MESSAGE, DELAYED, PBDEL} State;

typedef struct _message
{
  struct _message *prev;
  struct _message *next;
  unsigned char data[32];
} Message;

// MessageQueue is essentially identical to the (process) queue, except it is for Message objects (structs)
typedef struct _messageQueue
{
  Message *head;
  Message *tail;
} MessageQueue;

extern void AddMessageToTail(MessageQueue *queue, Message *message);
extern Message *DequeueMessageHead (MessageQueue *queue);

typedef struct  _process
{
  unsigned int program_address;
  struct  _process    *prev;
  struct  _process    *next;
  int     pid;
  State   state;
  unsigned int interrupt_delay;
  void    *user_stack_pointer;

  struct _messageQueue m_queue;
} Process;

extern void QuenosNewProcess (void (*entry_point) (void), char *stack_bottom, int stack_size);
extern void ShowReadyQueue (void);
extern void QuenosDispatch (void);

extern unsigned int process_stack_pointer;
extern unsigned int ksp;

#endif /* _QCORE_H_ */

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

typedef enum {Ready, Running, Blocked} State;

typedef struct  _pdb
{
        struct  _pdb    *prev;
        struct  _pdb    *next;
        int     pid;
        State   state;
        void    *SP;		/* saves user stack pointer when not running */
} PDB;

#define MAX_PDB 16

extern  void    QuerkNewProcess (void (*entry_point) (void),
				 char *stack_bottom, int stack_size);

extern  void    QuerkSWIHandler (void);

extern  void    QuerkDispatch (void);

#endif /* _QCORE_H_ */

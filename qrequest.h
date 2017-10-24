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

typedef enum    {Relinquish, BlockSelf, Unblock} Request;

extern	void    QuerkRelinquish (void);
extern	void    QuerkBlockSelf (void);
extern	void    QuerkUnblock (int other_pid);

#endif _QREQUEST_H_


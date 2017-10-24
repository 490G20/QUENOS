#ifndef _DBUG12_H_
#define _DBUG12_H_

/******************************************************************************
NAME:		DBug12.h
DESCRIPTION:	Useful declarations for accessing routines in DBug-12 monitor
		from Motorola. Adapted from Motorola application notes.
		This code handles DBug-12 versions >= 1.0.4 (HC12A4)
		or >= 2.0.0 (HC912B32).

******************************************************************************/

/*---- select one of the following ----*/

/* #define Version 104 */ /*--- for Motorola 68HC12A4EVB ---*/
#define Version 202 /*--- for Motorola 68HC912B32EVB, Axiom CME12B32 ---*/

/*----------------------------------------------------------------*/

/* typedefs used by D-Bug12 */

typedef char    *Address;
typedef int     Boolean;
typedef unsigned char   Byte;

/*----------------------------------------------------------------*/

/* identifies which vector for modification */

typedef enum    Vect
{
        UserPortHKWU    = 7,
        UserPortJKWU    = 8,
        UserAtoD        = 9,
        UserSCI1        = 10,
        UserSCI0        = 11,
        UserSPI0        = 12,
        UserPAccEdge    = 13,
        UserPAccOvf     = 14,
        UserTimerOvf    = 15,
        UserTimerCh7    = 16,
        UserTimerCh6    = 17,
        UserTimerCh5    = 18,
        UserTimerCh4    = 19,
        UserTimerCh3    = 20,
        UserTimerCh2    = 21,
        UserTimerCh1    = 22,
        UserTimerCh0    = 23,
        UserRTI         = 24,
        UserIRQ         = 25,
        UserXIRQ        = 26,
        UserSWI         = 27,
        UserTrap        = 28,
        RAMVectAddr     = -1
} Vect;

/*----------------------------------------------------------------*/

/* structure defining functions in D-Bug12 pointer table */

typedef struct
{
        void    (*DB12main) (void);
        int     (*DB12getchar) (void);
        int     (*DB12putchar) (int);
        int     (*DB12printf) (const char *, ...);
        int     (*GetCmdLine) (char *CmdLineStr, int CmdLineLen);
        char    * (*sscanhex) (char *HexStr, unsigned int *BinNum);
        int     (*DB12isxdigit) (int c);
        int     (*DB12toupper) (int c);
        int     (*DB12isalpha) (int c);
        unsigned int (*DB12strlen) (const char *cs);
        char    * (*DB12strcpy) (char *s1, char *s2);
        void    (*out2hex) (unsigned int num);
        void    (*out4hex) (unsigned int num);
        int     (*SetUserVector) (Vect VectNum, Address UserAddress);
        Boolean (*WriteEEByte) (Address EEAddress, Byte EEData);
        int     (*EraseEE) (void);
        int     (*ReadMem) (Address StartAddress, Byte *MemDataP,
                 unsigned int NumBytes);
        int     (*WriteMem) (Address StartAddress, Byte *MemDataP,
                 unsigned int NumBytes);
} DBug12FNStruct, *DBug12FNStructP;

/*----------------------------------------------------------------*/

/* select appropriate jump table location based on DBug-12 version */

#if Version < 200

#define DBug12FNP ((DBug12FNStructP) 0xfe00)    /* for Version 1.x.x */

#else

#define DBug12FNP ((DBug12FNStructP) 0xf680)    /* for Version 2.x.x */

#endif

/*----------------------------------------------------------------*/

/* utility typedefs */

#define DB12main DBug12FNP->DB12main
#define printf  DBug12FNP->DB12printf
#define getchar DBug12FNP->DB12getchar
#define putchar DBug12FNP->DB12putchar
#define isxdigit DBug12FNP->DB12isxdigit
#define toupper DBug12FNP->DB12toupper
#define isalpha DBug12FNP->DB12isalpha
#define strlen  DBug12FNP->DB12strlen
#define strcpy  DBug12FNP->DB12strcpy
#define GetCmdLine DBug12FNP->GetCmdLine
#define out2hex DBug12FNP->out2hex
#define out4hex DBug12FNP->out4hex
#define SetUserVector DBug12FNP->SetUserVector
#define WriteEEByte DBug12FNP->WriteEEByte
#define EraseEE DBug12FNP->EraseEE
#define ReadMem DBug12FNP->ReadMem
#define WriteMem DBug12FNP->WriteMem

#endif /* _DBUG12_H_ */


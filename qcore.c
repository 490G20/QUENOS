/******************************************************************************
NAME:		qcore.c
DESCRIPTION:	The core of the QUERK kernel. Includes the software interrupt
		handler, code to create new processes, and code to perform the
		kernel services.

		(C) 1999 by Naraig Manjikian
		Department of Electrical and Computer Engineering
		Queen's University
******************************************************************************/

#include "qcore.h"
#include "queue.h"
#include "nios2_ctrl_reg_macros.h"

static Process *running_process;

static Queue ready_queue;

volatile int* JTAG_UART_ptr = (int*) 0x10001000;// JTAG UART address
volatile int * interval_timer_ptr = (int *) 0x10002000;

static Process process_array[MAX_NUM_OF_PROCESSES];

static char kernel_stack[512];

static int num_of_processes = 0;

static int need_dispatch;

typedef enum    {Relinquish, BlockSelf, Unblock, TimerDelay, TimerInterrupt } Request;

extern unsigned int process_stack_pointer;
extern unsigned int ksp = (unsigned int) &kernel_stack[511];

void	put_jtag( volatile int* JTAG_UART_ptr, char c )
{
	while ((*(JTAG_UART_ptr + 1) & 0xFFFF0000) == 0)
		;
	*(JTAG_UART_ptr) = c;
}

void printString(char *text_string )
{
    int i =0;
    for(i = 0; text_string[i] != 0; ++i){ // print a text string
        put_jtag (JTAG_UART_ptr, text_string[i]);
    }
}

/**
 *  Prints out the ready queue, used for debugging
 */
void showReadyQueue (void) {
	Process * p = ready_queue.head;
	printString("queue: ");
	while (p!=0){
		put_jtag(JTAG_UART_ptr,'0'+p->pid);
		p = p->next;
	}
	put_jtag(JTAG_UART_ptr,'\n');
}


/* function to create a new process and add it to the ready queue;
   initial contents of stack are set using stackframe structure */
void QuenosNewProcess (void (*entry_point) (void), char *stack_bottom, int stack_size)
{
  //stackframe pointer deleted but not replaced?
  int new_pid = num_of_processes++;	/* assign new pid */
  Process* new_process = &process_array[new_pid];	/* pointer to descriptor */

  //create new process
  new_process->pid = new_pid;
  new_process->state = Ready;
  new_process->delay_time = 0;
  new_process->user_stack_pointer = stack_bottom + stack_size - 32;
  new_process->program_address = (unsigned int) entry_point;

  // Set ea value in process stack
  unsigned int* p = (unsigned int*) new_process->user_stack_pointer + 29;
  *p = new_process->program_address;

  unsigned int temp_sp;
  temp_sp = new_process->user_stack_pointer;

  asm ("movia r12, temp_sp");
  asm ("ldw r13, 0(r12)");
  asm ("stw gp, 104(r13)");

  printString("NP \n");
  showReadyQueue();
  
  AddToTail (&ready_queue, new_process);
  
  showReadyQueue();
}

/* functions for kernel services */
static int QuenosCoreBlockSelf (void)
{
  running_process->state = Blocked;
  return 1; /* need dispatch of new process */
}

static int QuenosCoreTimerBlockSelf (void)
{
  running_process->state = TimerDelay;
  return 1; /* need dispatch of new process */
}

static int QuenosCoreUnblock (int other_pid)
{
  if (process_array[other_pid].state == Blocked)
  {
    /* only unblock and add to ready queue if it was blocked */
    process_array[other_pid].state = Ready;
    AddToTail (&ready_queue, &process_array[other_pid]);
  }
  return 0; /* no dispatch of new process */
}

static int QuenosCoreTimerUnblock (void) 
{
  unsigned int dispatchNeeded = 1;
  for (int i = 0; i <= num_of_processes; i++)
  {
    if (process_array[i].state == TimerDelay &&  process_array[i].delay_time == 0)
    {
      /* Find the process that caused this timer interrupt */
      running_process->state = Ready;
      AddToHead (&ready_queue, running_process);
      running_process = &process_array[i];
      running_process->state = Running;
      dispatchNeeded = 0;
    }  
  }
  /*  
  *   Dispatch is needed if no process' interval is done 
  *   i.e. dispatch is needed if interrupt happens through
  *   round-robin selection
  */
  return dispatchNeeded; 
}
/* software interrupt routines */

/* The variables below should ideally be automatic within the two
   following functions, but the compiler adjusts the stack pointer
   at the start/end of each function if there are automatic variables.
   To make sure that we retain control over the stack pointer, the
   variables are made static outside the functions. */

// This is called directly called by the DE2 hardware when a hardware interrupt occurs,
// It is also called indirectly by the_exception(), the software interrupt
void interrupt_handler (void)
{
	printString("i\n");
  // First task: Update process control block for running process with stackpointer
  running_process->user_stack_pointer = (void*) process_stack_pointer;
  unsigned int* casted_prev_sp = (unsigned int*) running_process->user_stack_pointer;

  int requestType = *(casted_prev_sp+5);

  int ipending;
  int counter;
  /* Third task: retrieve arguments for kernel call. */
  /* They are available on the user process stack. */
  ipending = NIOS2_READ_IPENDING(); // Read the interrupt

  if (ipending) {
  // This currently has no actions, but this will never be called. It will always go to the else since there are no hardware interrupts yet
  }
  else {
  	if (requestType == Relinquish) {
		printString("r\n");
		showReadyQueue();
  		running_process->state = Ready;
  		AddToTail(&ready_queue, running_process);
  		need_dispatch = 1;       /* need dispatch of new process */
		showReadyQueue();
  	}
  	else if (requestType == BlockSelf) {
		printString("blk\n");
		showReadyQueue();
  		need_dispatch = QuenosCoreBlockSelf();
		showReadyQueue();
  	}
  	else if (requestType == Unblock){
  		int other_pid = *(casted_prev_sp+4);
		printString("unblk\n");
		showReadyQueue();
  		need_dispatch = QuenosCoreUnblock(other_pid);
		showReadyQueue();
  	}
	else if (requestType == TimerDelay){
		unsigned int delay = *(casted_prev_sp+4);
    unsigned int current_timer = *(interval_timer_ptr + 0x3)+ (*(interval_timer_ptr + 0x3) << 16);
    running_process->delay_time = delay - current_timer;
		printString("timed\n");
		showReadyQueue();
  	need_dispatch = QuenosCoreTimerBlockSelf();
		showReadyQueue();
		
	}
	else if (requestType == TimerInterrupt)
	{
		printString("ti\n");
		showReadyQueue();
  		running_process->state = Ready;
  		AddToHead(&ready_queue, running_process);
  		need_dispatch = QuenosCoreTimerUnblock();       /* need dispatch of new process */
      showReadyQueue();
		/* Set new timer interval ****************/
		// Find the least remaining wait time
    int min = INT_MAX;
    for (int i = 0; i <= num_of_processes; i++)
    {
      if (process_array[i].delay_time < min)
      {
        min = process_array[i].delay_time;
      }
    }
    if (min > msec)
    {
      counter = min * 50 * 1000000; //0x190000; // 1/(50 MHz) × (0x190000) = 33 msec
    }
    else 
    {
      counter = msec * 50 * 1000000; //0x190000; // 1/(50 MHz) × (0x190000) = 33 msec
    }
  
    *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
    *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
    *(interval_timer_ptr + 1) = 0x7; 		// STOP = 0, START = 1, CONT = 1, ITO = 1
    
    for (int i = 0; i <= num_of_processes; i++)
    {
      if (process_array[i].delay_time > 0)
        {
          process_array[i].delay_time = process_array[i].delay_time - counter;
        }
    }
	}
	else if (TIMER_INTERUPT) {
		// Save context by making software interrupt
	}

  }

  /* Fifth task: decide if dispatching of new process is needed. */
  if (need_dispatch)
  {
  	running_process = DequeueHead(&ready_queue);
  	running_process->state = Running;
	
	printString("CP: ");
	put_jtag(JTAG_UART_ptr,'0'+running_process->pid);
	put_jtag(JTAG_UART_ptr,'\n');
  }

  process_stack_pointer = (unsigned int) running_process->user_stack_pointer; // This will need to be checked in the debugger
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. It is declared as an interrupt function only
   to generate a return-from-interrupt instruction. */
void QuenosDispatch (void)
{
  printString("QD\n");
  showReadyQueue();
  
  running_process = DequeueHead (&ready_queue);
  running_process->state = Running;
  
  showReadyQueue();
  unsigned int sp_of_first_process;
    sp_of_first_process = (unsigned int)running_process->user_stack_pointer;
  asm("movia r12, sp_of_first_process");
  asm("ldw sp, 0(r12)");
  asm("ldw ea, 116(sp)");
  asm("addi sp, sp, 128");

  asm("eret");
}


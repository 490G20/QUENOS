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
#include "quser.h"
#include "nios2_ctrl_reg_macros.h"

static Process *running_process;

static Queue ready_queue;

volatile unsigned int* JTAG_UART_ptr = (unsigned int*) 0x10001000;// JTAG UART address
volatile int* interval_timer_ptr = (int*) 0x10002000;// interval timer base address

static Process process_array[MAX_NUM_OF_PROCESSES];

static char kernel_stack[512];

static int num_of_processes = 0;

static int need_dispatch;

extern unsigned int process_stack_pointer;
extern unsigned int ksp = (unsigned int) &kernel_stack[511];

unsigned int temp_sp;
unsigned int sp_of_first_process;

// Prints the character given in the parameter into JTAG output
void put_jtag(volatile unsigned int* JTAG_UART_ptr, unsigned int c)
{
  while ((*(JTAG_UART_ptr + 1) & 0xFFFF0000) == 0)
      ;
  *(JTAG_UART_ptr) = c;
}

// Prints the text string given to JTAG output
void printString(char *text_string)
{
  int i =0;
  for(i = 0; text_string[i] != 0; ++i){ // print a text string
    put_jtag (JTAG_UART_ptr, text_string[i]);
  }
}

// Prints out the ready queue, used for debugging
void ShowReadyQueue(void) {
  Process * p = ready_queue.head;
  printString("queue: ");
  while (p!=0){
    put_jtag(JTAG_UART_ptr,'0'+p->pid);
    p = p->next;
  }
  put_jtag(JTAG_UART_ptr,'\n');
}


/* Function to create a new process and add it to the ready queue;
   initial contents of stack are set using stackframe structure */
void QuenosNewProcess(void (*entry_point) (void), char *stack_bottom, int stack_size)
{
  // Assign a new pid for the process
  int new_pid = num_of_processes++;
  Process* new_process = &process_array[new_pid];

  // Initialize empty message
  MessageQueue mq;
  mq.head = 0;
  mq.tail = 0;

  // Create new process
  new_process->pid = new_pid;
  new_process->interrupt_delay = 0;
  new_process->state = READY;
  new_process->user_stack_pointer = stack_bottom + stack_size - 32;
  new_process->program_address = (unsigned int) entry_point;
  new_process->m_queue = mq;

  // Set ea (return address) value in process stack so that the kernel knows where the process's code starts
  unsigned int* p = (unsigned int*) new_process->user_stack_pointer + 29;
  *p = new_process->program_address;

  // Set gp (global pointer) value in process stack in order to fake out the initializing of the process
  temp_sp = new_process->user_stack_pointer;
  asm ("movia r12, temp_sp");
  asm ("ldw r13, 0(r12)");
  asm ("stw gp, 104(r13)");

  // Add the process to the ready queue
  AddToTail (&ready_queue, new_process);
}

/* Kernel Functions */

// Kernel function to block the current process
static int QuenosCoreBlockSelf(void)
{
  running_process->state = BLOCKED;
  return 1; /* need dispatch of new process */
}

// Kernel function to unblock the target process using the current process
static int QuenosCoreUnblock(int other_pid)
{
  if (process_array[other_pid].state == BLOCKED)
  {
    /* only unblock and add to ready queue if it was blocked */
    process_array[other_pid].state = READY;
    AddToTail (&ready_queue, &process_array[other_pid]);
  }
  return 0; /* no dispatch of new process */
}

// Kernel function to send a message from the current process to the target process
static void QuenosCoreSendMessage(int target_pid, Message *m){
  if (process_array[target_pid].state == WAITING_FOR_MESSAGE)
  {
    // only unblock and add to ready queue if it was blocked
    process_array[target_pid].state = READY;
    AddToTail (&ready_queue, &process_array[target_pid]);
  }
}

// Kernel function to set the current process's state to wait for a timer interrupt
static int QuenosCoreTimerDelay (void)
{
  running_process->state = DELAYED;
  return 1;
}

// Kernel function to change the target process's state from delayed (waiting for timer)
// to ready, essentially unblocking the target process.
static int QuenosCoreTimerUnblock(int other_pid)
{
  if (process_array[other_pid].state == DELAYED)
  {
    /* only unblock and add to ready queue if it was blocked */
    process_array[other_pid].state = READY;
    AddToTail (&ready_queue, &process_array[other_pid]);
  }
  return 0;
}

// Kernel function to block current process by changing it's state to pushbutton delay. This means that
// the process will not be added to the ready queue until a pushbutton interrupt has occurred.
static int QuenosCorePBBlockSelf(void)
{
  // This is a special push button type delay such that it does not get confused with a regular SW delay
  running_process->state = PBDEL;
  return 1;
}

// Kernel function to change the target process's state from pushbutton delay (waiting for pushbutton)
// to ready, essentially unblocking the target process.
static int QuenosCorePBUnblock(int other_pid)
{
  if (process_array[other_pid].state == PBDEL)
  {
    // Only if a button was pushed then unblock this process to write a "pressed" string to the console
    process_array[other_pid].state = READY;
    AddToTail (&ready_queue, &process_array[other_pid]);
  }
  return 0;
}

/* Software Interrupt Routines */

// This is called directly called by the DE2 hardware when a hardware interrupt occurs,
// It is also called indirectly by the_exception(), the software interrupt
void interrupt_handler (void)
{
    int ipending;
    extern volatile int key_pressed;

    // First task: Update process control block for running process with stackpointer
    running_process->user_stack_pointer = (void*) process_stack_pointer;
    unsigned int* casted_prev_sp = (unsigned int*) running_process->user_stack_pointer;

    // Get the requestType (the type of interrupt ex. block, unblock, etc) by grabbing the value off of memory using
    // the stack pointer value offset by 5 (since the requestType is put into register 5)
    int requestType = *(casted_prev_sp+5);

    // Read the interrupt
    ipending = NIOS2_READ_IPENDING();

    if (ipending)
    {
      if (ipending & 0x1)
      {
        // clear the interrupt
	*(interval_timer_ptr) = 0;

        // Find the process with the timerdelay
        int i = 0;
        while (i <= num_of_processes)
        {
          int previous_interval = *(interval_timer_ptr + 0x2)+ (*(interval_timer_ptr + 0x3) << 16);
          if (process_array[i].state == DELAYED)
          {
            process_array[i].interrupt_delay = 0;
            need_dispatch = QuenosCoreTimerUnblock(i);
	    break;
          }
          i++;
        }
      }

      if (ipending & 0x2){
        volatile int * KEY_ptr = (int *) 0x10000050;
        volatile int * slider_switch_ptr = (int *) 0x10000040;
        volatile int * green_LED_ptr = (int *)0x10000010;
        int press;

        press = *(KEY_ptr + 3); // read the pushbutton interrupt register
        *(KEY_ptr + 3) = 0; // clear the interrupt

        if (press & 0x2)
        {
          *(green_LED_ptr) = 4;
        }
        else if (press & 0x4)
        {
          *(green_LED_ptr) = 16;
        }
        else // press & 0x8, which is KEY3
        {
          *(green_LED_ptr) = 64;
        }

        int i = 0;
        while (i <= num_of_processes)
        {
          if (process_array[i].state == PBDEL)
          {
            need_dispatch = QuenosCorePBUnblock(i);
            break;
          }
        i++;
        }
      }
    }
    else
    {
      // Kernel handling each different request type
      if (requestType == RELINQUISH)
      {
        // Relinquish simply puts the current process back onto the ready queue
        running_process->state = READY;
        AddToTail(&ready_queue, running_process);

        // Dispatches new process
        need_dispatch = 1;
      }
      else if (requestType == BLOCK_SELF)
      {
        need_dispatch = QuenosCoreBlockSelf();
      }
      else if (requestType == UNBLOCK)
      {
        // Get the target_pid (the target process pid) by grabbing the value off of memory using
        // the stack pointer value of the process offset by 4 (since the other_pid is put into register 4)
        int target_pid = *(casted_prev_sp+4);
        need_dispatch = QuenosCoreUnblock(target_pid);
      }
      else if (requestType == SEND_MESSAGE)
      {
        int target_pid = *(casted_prev_sp+4);
        Message *messageToSend = *(casted_prev_sp+6);

        // Add the message to the message queue of the target process
        AddMessageToTail(&process_array[target_pid].m_queue, messageToSend);

        if (process_array[target_pid].state == WAITING_FOR_MESSAGE)
        {
          // If the target process was blocked waiting for a message, put the target process back onto the ready queue
          process_array[target_pid].state = READY;
          AddToTail (&ready_queue, &process_array[target_pid]);

          // Set up the return of the target process by putting the message at the top of the message queue into the
          // memory address of register 2 (returning to target process will get the message off register 2)
          Message *current_message;
          unsigned int* casted_target_sp = (unsigned int*) process_array[target_pid].user_stack_pointer;
          current_message = DequeueMessageHead(&process_array[target_pid].m_queue);
          *(casted_target_sp+2) = current_message;
        }

        // Put the current process back onto the ready queue and dispatch new process
        running_process->state = READY;
        AddToTail(&ready_queue, running_process);
        need_dispatch = 1;
      }
      else if (requestType == READ_MESSAGE)
      {
        need_dispatch = 0;
        Message *current_message;
        current_message = DequeueMessageHead(&running_process->m_queue);

        if (current_message != 0)
        {
          // If there is a message in the message queue, add the current process back onto the ready queue
          running_process->state = READY;
          AddToTail(&ready_queue, running_process);

          // Store address of current message into where the expected saved value of r2
          *(casted_prev_sp+2) = current_message;
          need_dispatch = 1;
        }
        else
        {
          // If there is no message, set the current process state to blocked waiting for message
          running_process->state = WAITING_FOR_MESSAGE;

          // Do not hog CPU, let another process run and recheck on next run
          need_dispatch = 1;
        }
      }
      else if (requestType == TIMER_DELAY)
      {
        // Set the timer using the time we passed in
        unsigned int msec = *(casted_prev_sp+4); //currently assumes only one process is running
        unsigned int counter = msec * 50000;

        *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
        *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
        *(interval_timer_ptr + 1) = 0x5;  // STOP = 0, START = 1, CONT = 0, ITO = 1

        running_process->interrupt_delay = counter;
        need_dispatch = QuenosCoreTimerDelay();
      }
      else if (requestType == PB_BLOCK)
      {
        need_dispatch = QuenosCorePBBlockSelf();
      }
    }

    // Decide if dispatching of new process is needed
    if (need_dispatch)
    {
      running_process = DequeueHead(&ready_queue);
      running_process->state = RUNNING;
    }

    // Set the stack pointer variable to the sp of the next process to run
    process_stack_pointer = (unsigned int) running_process->user_stack_pointer;
}

/* The following function is called _directly_ (i.e., _not_ through an
   interrupt) from the main program only once to select the first
   ready process to run. */
void QuenosDispatch (void)
{
  running_process = DequeueHead (&ready_queue);
  running_process->state = RUNNING;

  sp_of_first_process = (unsigned int)running_process->user_stack_pointer;

  // Restore the stack pointer and return value from memory into the appropriate registers
  asm("movia r12, sp_of_first_process");
  asm("ldw sp, 0(r12)");
  asm("ldw ea, 116(sp)");
  asm("addi sp, sp, 128");

  // Return from interrupt
  asm("eret");
}


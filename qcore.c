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

extern volatile int* JTAG_UART_ptr = (int*) 0x10001000;// JTAG UART address
volatile int* interval_timer_ptr = (int*) 0x10002000;// interval timer base address

static Process process_array[MAX_NUM_OF_PROCESSES];

static char kernel_stack[512];

static int num_of_processes = 0;

static int need_dispatch;

extern unsigned int process_stack_pointer;
extern unsigned int ksp = (unsigned int) &kernel_stack[511];

unsigned int temp_sp;
unsigned int sp_of_first_process;

/* Below functions are for utility and demo purpouses */

void put_jtag(volatile int* JTAG_UART_ptr, char c)
{
    while ((*(JTAG_UART_ptr + 1) & 0xFFFF0000) == 0)
        ;
    *(JTAG_UART_ptr) = c;
}

void printString(char *text_string)
{
    int i =0;
    for(i = 0; text_string[i] != 0; ++i){ // print a text string
        put_jtag (JTAG_UART_ptr, text_string[i]);
    }
}

/**
 *  Prints out the ready queue, used for debugging
 */
void showReadyQueue (void) 
{
    Process * p = ready_queue.head;
    printString("queue: ");
    while (p!=0){
            put_jtag(JTAG_UART_ptr,'0'+p->pid);
            p = p->next;
    }
    put_jtag(JTAG_UART_ptr,'\t');
    put_jtag(JTAG_UART_ptr,'\t');
}


/* function to create a new process and add it to the ready queue;
   initial contents of stack are set using stackframe structure */
void QuenosNewProcess (void (*entry_point) (void), char *stack_bottom, int stack_size)
{
    //stackframe pointer deleted but not replaced?
    int new_pid = num_of_processes++;	/* assign new pid */
    Process* new_process = &process_array[new_pid];	/* pointer to descriptor */

    //initialize empty message
    MessageQueue mq;
    mq.head = 0;
    mq.tail = 0;

    //create new process
    new_process->pid = new_pid;
    new_process->interrupt_delay = 0;
    new_process->state = READY;
    new_process->user_stack_pointer = stack_bottom + stack_size - 32;
    new_process->program_address = (unsigned int) entry_point;
    new_process->m_queue = mq;

    // Set ea value in process stack
    unsigned int* p = (unsigned int*) new_process->user_stack_pointer + 29;
    *p = new_process->program_address;

    temp_sp = new_process->user_stack_pointer;

    asm ("movia r12, temp_sp");
    asm ("ldw r13, 0(r12)");
    asm ("stw gp, 104(r13)");

    //showReadyQueue();

    AddToTail (&ready_queue, new_process);

    //showReadyQueue();
}

/* functions for kernel services */
static int QuenosCoreBlockSelf (void)
{
    running_process->state = BLOCKED;
    return 1; /* need dispatch of new process */
}

// may need header in header file for use in other method?
static void QuenosCoreSendMessage(int target_pid, Message *m){
	// Shove message (address?) somewhere kernel can get at it

	if (process_array[target_pid].state == WAITING_FOR_MESSAGE){
		/* only unblock and add to ready queue if it was blocked */
        process_array[target_pid].state = READY;
        AddToTail (&ready_queue, &process_array[target_pid]);
    }

	// Relinquish kernel here or in the IH preferable?

	// Dispatch something
}

static int QuenosCoreUnblock (int other_pid)
{
    if (process_array[other_pid].state == BLOCKED)
    {
        /* only unblock and add to ready queue if it was blocked */
        process_array[other_pid].state = READY;
        AddToTail (&ready_queue, &process_array[other_pid]);
    }
    return 0; /* no dispatch of new process */
}

static int QuenosCoreTimerDelay (void)
{
    running_process->state = DELAYED;
    return 1; /* need dispatch of new process */
}

static int QuenosCoreTimerUnblock (int other_pid)
{
    if (process_array[other_pid].state == DELAYED)
    {
        /* only unblock and add to ready queue if it was blocked */
        process_array[other_pid].state = READY;
        AddToTail (&ready_queue, &process_array[other_pid]);
    }
    return 0; /* no dispatch of new process */
}

static int QuenosCorePBBlockSelf(void){
	running_process->state = PBDEL; /*This is a special push button type delay such that it does not get confused with a regular SW delay*/
	return 1;
	/* Need Dispatch of new process */
}

static int QuenosCorePBUnblock (int other_pid)
{
    if (process_array[other_pid].state == PBDEL)
    {
        /* only if a button was pushed then unblock this process to write a "pressed" string to the console */
        process_array[other_pid].state = READY;
        AddToTail (&ready_queue, &process_array[other_pid]);
    }
    return 0; /* no dispatch of new process */
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
  #ifndef DEMO
    printString("i\n");
    #endif
	// First task: Update process control block for running process with stackpointer
    running_process->user_stack_pointer = (void*) process_stack_pointer;
    unsigned int* casted_prev_sp = (unsigned int*) running_process->user_stack_pointer;

    int requestType = *(casted_prev_sp+5);

    int ipending;
	extern volatile int key_pressed;
    /* Third task: retrieve arguments for kernel call. */
    /* They are available on the user process stack. */
    ipending = NIOS2_READ_IPENDING(); // Read the interrupt

    if (ipending) {
    // This currently has no actions, but this will never be called. It will always go to the else since there are no hardware interrupts yet
      if (ipending & 0x1){
		*(interval_timer_ptr) = 0; // clear the interrupt
        // Find the process with the timerdelay
        int i = 0;
        while (i <= num_of_processes){
          int previous_interval = *(interval_timer_ptr + 0x2)+ (*(interval_timer_ptr + 0x3) << 16);
           if (process_array[i].state == DELAYED){// &&  process_array[i].interrupt_delay == previous_interval){
             process_array[i].interrupt_delay = 0;
			 printString("Timer unblock\n");
             need_dispatch = QuenosCoreTimerUnblock(i);
			 break;
           }
           i++;
        }      
      }
	  if (ipending & 0x2){
		  
		  //Taken from the altera DE2-115 documentation
				volatile int * KEY_ptr = (int *) 0x10000050;
				volatile int * slider_switch_ptr = (int *) 0x10000040;
				volatile int * green_LED_ptr = (int *)0x10000010;
				int press;
				
				press = *(KEY_ptr + 3); // read the pushbutton interrupt register
				*(KEY_ptr + 3) = 0; // clear the interrupt
				
				if (press & 0x2) // KEY1
				{
				printString("PUSHED 1!\n");
				*(green_LED_ptr) = 4;
				}
				//key_pressed = 1;
				else if (press & 0x4) // KEY2
				{
				printString("PUSHED 2!\n");
				*(green_LED_ptr) = 16;
				}
				//key_pressed = 2;
				else // press & 0x8, which is KEY3
				{
				printString("PUSHED 3!\n");
				*(green_LED_ptr) = 64;
				}
				
		        int i = 0;
		        while (i <= num_of_processes){
		          	if (process_array[i].state == PBDEL){
		             need_dispatch = QuenosCorePBUnblock(i);
					 break;
		           }
		           i++;
				}
	  }
    }
    else {
            if (requestType == RELINQUISH) {
              #ifndef DEMO
                printString("r\n");
                #endif
                //showReadyQueue(); // Versioning bug: ready queue used to be garbage when dealing with null process
                running_process->state = READY;
                AddToTail(&ready_queue, running_process);
                need_dispatch = 1;       /* need dispatch of new process */
                showReadyQueue();
            }
            else if (requestType == BLOCK_SELF) {
              #ifndef DEMO
                printString("blk\n");
                #endif
                //showReadyQueue();
                need_dispatch = QuenosCoreBlockSelf();
                showReadyQueue();
            }
			else if (requestType == PB_BLOCK) {
        #ifndef DEMO
                printString("PBBlock\n");
                #endif
                //showReadyQueue();
                need_dispatch = QuenosCorePBBlockSelf();
                showReadyQueue();
            }
            else if (requestType == UNBLOCK){
                int other_pid = *(casted_prev_sp+4);
                #ifndef DEMO
                printString("unblk\n");
                #endif
                //showReadyQueue();
                need_dispatch = QuenosCoreUnblock(other_pid);
                showReadyQueue();
            }
            else if (requestType == SEND_MESSAGE){
              #ifndef DEMO
                printString("send\n");
                #endif
                //showReadyQueue();
                int target_pid = *(casted_prev_sp+4);
                Message *messageToSend = *(casted_prev_sp+6);
                AddMessageToTail(&process_array[target_pid].m_queue, messageToSend);

                if (process_array[target_pid].state == WAITING_FOR_MESSAGE){
                    process_array[target_pid].state = READY;
                    AddToTail (&ready_queue, &process_array[target_pid]);


                    Message *current_message;
                    unsigned int* casted_target_sp = (unsigned int*) process_array[target_pid].user_stack_pointer;
                    current_message = DequeueMessageHead(&process_array[target_pid].m_queue);
                    *(casted_target_sp+2) = current_message;
                }

                running_process->state = READY;
                AddToTail(&ready_queue, running_process);
                showReadyQueue();
	    }
            else if (requestType == READ_MESSAGE){
              #ifndef DEMO
                printString("readin\n");
                #endif
                //showReadyQueue();
                need_dispatch = 0;
                Message *current_message;
                current_message = DequeueMessageHead(&running_process->m_queue);

                if (current_message != 0){
                    running_process->state = READY;
                    AddToTail(&ready_queue, running_process);
                    // Store address of current message into where the expected saved value of r2
                    *(casted_prev_sp+2) = current_message;
                }
                else {
                    running_process->state = WAITING_FOR_MESSAGE;
                    // Do not hog CPU, let another process run and recheck on next run
                    need_dispatch = 1;
                }

                showReadyQueue();
	      }
        else if (requestType == TIMER_DELAY){
          #ifndef DEMO
				printString("timer delay\n");
        #endif
                // Set the timer using the time we passed in 
                unsigned int msec = *(casted_prev_sp+4); //currently assumes only one process is running
                unsigned int counter = msec * 50000;
				//counter = 0x190000;
                *(interval_timer_ptr + 0x2) = (counter & 0xFFFF);
                *(interval_timer_ptr + 0x3) = (counter >> 16) & 0xFFFF;
                *(interval_timer_ptr + 1) = 0x5;  // STOP = 0, START = 1, CONT = 0, ITO = 1
                
                running_process->interrupt_delay = counter;
                
				need_dispatch = QuenosCoreTimerDelay();
        showReadyQueue();
                
        }
    }

    /* Fifth task: decide if dispatching of new process is needed. */
    if (need_dispatch)
    {
          running_process = DequeueHead(&ready_queue);
          running_process->state = RUNNING;
          printString("Current Process: ");
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
  #ifndef DEMO
    printString("QD\n");
    #endif
    //showReadyQueue();

    running_process = DequeueHead (&ready_queue);
    running_process->state = RUNNING;

    //showReadyQueue();
    sp_of_first_process = (unsigned int)running_process->user_stack_pointer;

    asm("movia r12, sp_of_first_process");
    asm("ldw sp, 0(r12)");
    asm("ldw ea, 116(sp)");
    asm("addi sp, sp, 128");

    asm("eret");
}


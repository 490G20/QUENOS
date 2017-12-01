.section .text
.global KernelRelinquish
#.type KernelRelinquish, @function
.global KernelBlock
#.type KernelBlockSelf, @function

.global KernelUnblock
#.type KernelUnblock, @function
.global OverwriteSP
.global UndoChangesToSP

# Refactor this file's name and method names
#request.s contains functions to set up relevant information when requesting kernel service.
# We enter the kernel using trap, and expect the ?exception handler? to automatically save everything onto the stack for us
# and restore it after

#TODO: Determine how to link and if qrequest.h
KernelRelinquish:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5,1 #relinquish enum
    trap
    ldw r5, 0(sp)
    addi sp,sp,4
	ret

KernelBlock:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5,2 #block self enum
    trap
    ldw r5, 0(sp)
    addi sp,sp,4
	ret

KernelUnblock:
    #Expect otherpid in r4 according to altera nios 2 application binary interface
    subi sp, sp, 8
    stw r5, 4(sp)
    stw r4, 0(sp)
    movi r5,3 #unblock enum
    trap
    ldw r5, 4(sp)
    ldw r4, 0(sp)
    addi sp,sp,8
	ret

OverwriteSP: # This method is to be run while sp pointers to previously running process stack
    subi sp, sp, 4
    stw r4, 0(sp) # save old r4 value on previously running process stack
    mov sp, r4 #write in new sp value

# TODO: rename
UndoChangesToSP: # This method is run while sp points to kernel stack, can have new desired stack pointer value on sp
    mov sp, r4 #write in the old sp value passed in r4
    ldw r4, 0(sp) # restore r4 from previous running process stack
    addi sp,sp,4
    mov sp, r5 #write in the new SP value passed in r5



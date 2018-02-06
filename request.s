.section .text

.global KernelRelinquish
#.type KernelRelinquish, @function
.global KernelBlock
#.type KernelBlock, @function
.global KernelUnblock
#.type KernelUnblock, @function
.global KernelSendMessage
.global KernelReadMessage


#qrequest.s contains functions to set up relevant information when requesting kernel service.
# We enter the kernel using trap, and expect the ?xception handler to automatically save everything onto the stack for us
# and restore it after

KernelRelinquish:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5,0 #relinquish enum, TODO: Ask if this enum should start at 1 for possibility of 0 being a common garbage value that could be misread
    trap
    ldw r5, 0(sp)
    addi sp,sp,4
	ret

KernelBlock:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5,1 #block self enum
    trap
    ldw r5, 0(sp)
    addi sp,sp,4
	ret

KernelUnblock:
    #Expect otherpid in r4 according to altera nios 2 application binary interface
    subi sp, sp, 8
    stw r5, 4(sp)
    stw r4, 0(sp)
    movi r5,2 #unblock enum
    trap
    ldw r5, 4(sp)
    ldw r4, 0(sp)
    addi sp,sp,8
	ret

# Expect target pid to message in r4, and target address in r5
KernelSendMessage:
    subi sp, sp, 12
    stw r6, 8(sp)
    stw r5, 4(sp)
    stw r4, 0(sp)
    mov r5, r6
    movi r5, 3 # send message enum
    trap
    ldw r6, 8(sp)
    ldw r5, 4(sp)
    ldw r4, 0(sp)
    addi sp,sp,12
    ret

KernelReadMessage:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5, 4 # read message enum
    trap
    # We will expect (and accept) that the interrupt handler method called by exception_handler.c will overwrite the values to load
    # into r2
    ldw r5, 0(sp)
    addi sp,sp,4
    ret

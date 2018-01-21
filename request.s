.section .text
.global KernelRelinquish
#.type KernelRelinquish, @function
.global KernelBlock
#.type KernelBlockSelf, @function

.global KernelUnblock
#.type KernelUnblock, @function

#.S extension required?

#qrequest.s contains functions to set up relevant information when requesting kernel service.
# We enter the kernel using trap, and expect the ?exception handler? to automatically save everything onto the stack for us
# and restore it after

#Could identical file name create issues?
#TODO: Determine how to link and if qrequest.h
KernelRelinquish:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5,1 #relinquish enum
    addi sp, sp, 4
    trap
    subi sp, sp, 4
    ldw r5, 0(sp)
    addi sp,sp,4
	ret

KernelBlock:
    subi sp, sp, 4
    stw r5, 0(sp)
    movi r5,2 #block self enum
    addi sp, sp, 4
    trap
    subi sp, sp, 4
    ldw r5, 0(sp)
    addi sp,sp,4
	ret

KernelUnblock:
    #Expect otherpid in r4 according to altera nios 2 application binary interface
    subi sp, sp, 8
    stw r5, 4(sp)
    stw r4, 0(sp)
    movi r5,3 #unblock enum
    addi sp,sp,8
    trap
    subi sp, sp, 8
    ldw r5, 4(sp)
    ldw r4, 0(sp)
    addi sp,sp,8
	ret

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
    stw r5, 4(sp)
    movi r5,1 #relinquish enum
    trap
    ldw r5, 4(sp)
	ret

KernelBlock:
    subi sp, sp, 4
    stw r5, 4(sp)
    movi r5,2 #block self enum
    trap
    ldw r5, 4(sp)
	ret

KernelUnblock:
    #Expect otherpid in r4 according to altera nios 2 application binary interface
    subi sp, sp, 8
    stw r5, 4(sp)
    stw r4, 8(sp)
    movi r5,3 #unblock enum
    trap
    ldw r5, 4(sp)
    ldw r4, 8(sp)
	ret

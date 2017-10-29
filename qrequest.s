#qrequest.s contains functions to set up relevant information when requesting kernel service.
# We enter the kernel using trap

#TODO: Determine how to link and if qrequest.h
KernelRelinquish:
    subi sp, sp, 4
    add r23,r23,r0
    addi r23,1 # Relinquish enum
    stw r23, 4(sp)
    trap
	ret

KernelBlockSelf:
    subi sp, sp, 4
    add r23,r23,r0
    addi r23,2 #block self enum
    stw r23, 4(sp)
    trap
	ret

# Read Altera documentation (application binary interface, connections for C compiler)
# about which register by convention is 1st
# argument and retrieve the other_pid from there to store
#
#How and where to put otherpid into r22 here or in qrequest.c?
KernelUnblock:
    #Expect otherpid in r22
    subi sp, sp, 8
    add r23,r23,r0
    addi r23,3
    stw r23, 4(sp)
    stw r22, 8(sp) #TODO: update this line of code to use the correct register
    trap
	ret

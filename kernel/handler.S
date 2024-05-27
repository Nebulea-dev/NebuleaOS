    .text
# Makes the tag public
    .globl IT_32_handler
# handler start
IT_32_handler:
# Saving important registers
    pushl %eax
    pushl %edx
    pushl %ecx

# Calling c function implementing the handler
    call tic_PIT
# restore important registers

    popl %ecx
    popl %edx
    popl %eax
# handler end
    iret

# Makes the tag public
    .globl IT_49_handler
# handler start
IT_49_handler:
# Saving important registers
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi

# Calling c function implementing the handler
    call __syscall
# restore important registers

    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
# we don't popl EAX because it contains the return value of the function

# handler end
    iret

# Makes the tag public
    .globl IT_14_handler
# handler start
IT_14_handler:
# Saving important registers
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi

# Calling c function implementing the handler
    call pagefault_interrupt
# restore important registers

    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
# we don't popl EAX because it contains the return value of the function

# handler end
    iret

    # Makes the tag public
    .globl IT_13_handler
# handler start
IT_13_handler:
# Saving important registers
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi

# Calling c function implementing the handler
    call gpf_interrupt
# restore important registers

    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
# we don't popl EAX because it contains the return value of the function

# handler end
    iret


    # Makes the tag public
    .globl IT_33_handler
# handler start
IT_33_handler:
# Saving important registers
    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi

# Calling c function implementing the handler
    call keyboard_interrupt
# restore important registers

    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx
# we don't popl EAX because it contains the return value of the function

# handler end
    iret

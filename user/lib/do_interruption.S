    .text
    .globl __do_interruption
# Structure de la pile en entree :
#   %esp + 4 : adresse du numero de l'interruption
#   %esp + 8 : adresse du tableau de paramètres de l'interruption
__do_interruption:
    # Parcours du tableau de paramètres de l'interruption

    movl 8(%esp), %eax

    pushl %ebx
    pushl %ecx
    pushl %edx
    pushl %esi
    pushl %edi

    movl 0(%eax), %ebx
    movl 4(%eax), %ecx
    movl 8(%eax), %edx
    movl 12(%eax), %esi
    movl 16(%eax), %edi

    # Numero de l'interruption
    # Offset 4 + 6*4 = 28
    movl 24(%esp), %eax 


    # interruption
	int $49

    # Restore registers
    popl %edi
    popl %esi
    popl %edx
    popl %ecx
    popl %ebx

    # Return
    ret


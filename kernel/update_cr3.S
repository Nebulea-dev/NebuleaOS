    .text
    .globl update_cr3
# Structure de la pile en entree :
#   %esp + 4 : adresse du nouveau cr3
update_cr3:
    # sauvegarde du contexte de l'ancien processus
    movl 4(%esp), %eax
    movl %eax, %cr3
    ret


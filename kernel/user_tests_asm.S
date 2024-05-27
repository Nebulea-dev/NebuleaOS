.text

.globl proc6_1
proc6_1:
movl $3,%eax
ret
.previous

.globl proc6_2
proc6_2:
movl 4(%esp),%eax
pushl %eax
popl %eax
ret
.previous

.globl proc6_3
proc6_3:
movl 4(%esp),%eax
pushl %eax
popl %eax
ret
.previous


